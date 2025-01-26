#include "Scene.h"
#include <algorithm>
#include <numbers>
#include <ranges>

void Scene::addModel(PModel model, glm::mat4 transform)
{
  model->transform(transform);
  models.push_back(std::move(model));
}

void Scene::addModels(std::vector<PModel> _models, glm::mat4 transform)
{
  for (auto& _model : _models)
  {
    _model->transform(transform);
    models.push_back(std::move(_model));
  }
}

void Scene::generate()
{
  std::vector<PNode> pendingNodes;
  while (!models.empty())
  {
    auto& model = models.back();
    ModelReference ref = {
        .positionOffset = (uint32_t)positionPool.size(),
        .indicesOffset = (uint32_t)indicesPool.size(),
        .numIndices = (uint32_t)model->indices.size(),
    };
    for (uint32_t i = 0; i < model->positions.size(); ++i)
    {
      positionPool.push_back(model->positions[i]);
      texCoordsPool.push_back(model->texCoords[i]);
    }
    for (uint32_t i = 0; i < model->indices.size(); ++i)
    {
      indicesPool.push_back(model->indices[i]);
      edgesPool.push_back(model->edges[i * 2 + 0]);
      edgesPool.push_back(model->edges[i * 2 + 1]);
      faceNormalsPool.push_back(model->faceNormals[i]);
    }
    pendingNodes.push_back(std::make_unique<Node>(model->boundingBox, ref));
    models.pop_back();
  }
  while (pendingNodes.size() > 1)
  {
    int lhs = pendingNodes.size();
    int rhs = pendingNodes.size();
    float minSurface = std::numeric_limits<float>::max();
    for (int i = 0; i < pendingNodes.size(); ++i)
    {
      for (int j = 0; j < pendingNodes.size(); ++j)
      {
        if (i == j)
          continue;
        AABB combined = AABB::combine(pendingNodes[i]->aabb, pendingNodes[j]->aabb);
        float surface = combined.surfaceArea();
        if (minSurface > surface)
        {
          lhs = i;
          rhs = j;
          minSurface = surface;
        }
      }
    }
    PNode newNode = std::make_unique<Node>(AABB::combine(pendingNodes[lhs]->aabb, pendingNodes[rhs]->aabb));
    newNode->left = std::move(pendingNodes[lhs]);
    newNode->right = std::move(pendingNodes[rhs]);
    assert(rhs > lhs);
    //
    pendingNodes.erase(pendingNodes.begin() + rhs);
    pendingNodes.erase(pendingNodes.begin() + lhs);
    pendingNodes.push_back(std::move(newNode));
  }
  hierarchy = std::move(pendingNodes[0]);
}

extern glm::vec3 rnd01;

void Scene::traceRay(Ray ray, Payload& payload, const float tmin, const float tmax) const noexcept
{
  auto results = generateIntersections(hierarchy, ray, tmin, tmax);
  float closestT = std::numeric_limits<float>::max();
  IntersectionInfo info;
  for (uint32_t i = 0; i < results.size(); ++i)
  {
    if (results[i].hitInfo.t < closestT)
    {
      closestT = results[i].hitInfo.t;
      info = results[i];
    }
  }
  if (closestT < std::numeric_limits<float>::max())
  {
    // russian roulette ray termination
    float p = std::max(std::max(info.brdf.albedo.x, info.brdf.albedo.y), info.brdf.albedo.z);

    if (payload.depth >= 12)
    {
      return;
    }
    else if (payload.depth > 5)
    {
      if (rnd01.z >= p)
        return;
      else
        payload.accumulatedMaterial /= p;
    }
    // emissive
    payload.accumulatedRadiance += payload.accumulatedMaterial * info.brdf.emissive * payload.emissive;
    payload.accumulatedMaterial *= info.brdf.albedo;

    // direct lighting
    for (const auto& d : directionalLights)
    {
      // if there is an intersection, the light is occluded so no lighting
      if (!testIntersection(hierarchy, Ray(info.hitInfo.position, -d.direction), 1e-4, 1e20))
      {
        payload.accumulatedRadiance += info.brdf.evaluate(info.hitInfo, -ray.direction, d.direction, d.color);
      }
    }
    for (const auto& p : pointLights)
    {
      glm::vec3 lightDir = p.position - info.hitInfo.position;
      // if (!testIntersection(hierarchy, Ray(info.hitInfo.position, -lightDir), 1e-4, 1))
      {
        float d = glm::length(lightDir);
        float illuminance = std::max(1 - d / p.attenuation, 0.0f);

        payload.accumulatedRadiance += illuminance * info.brdf.evaluate(info.hitInfo, -ray.direction, lightDir, p.color);
      }
    }

    // TODO: Next Event Estimation for mesh lights

    // indirect lighting
    float r1 = 2 * std::numbers::pi * rnd01.x;
    float r2 = rnd01.y;
    float r2s = sqrt(r2);
    glm::vec3 w = info.hitInfo.normalLight;
    glm::vec3 u = glm::normalize(glm::cross(std::abs(w.x) > 0.1 ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0), w));
    glm::vec3 v = glm::cross(w, u);
    ray = Ray(info.hitInfo.position, glm::normalize(u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2)));
    payload.emissive = 0;
    payload.depth++;
    traceRay(ray, payload, tmin, tmax);
  }
}

bool Scene::testIntersection(const PNode& currentNode, const Ray ray, const float tmin, float tmax) const noexcept
{
  if (!currentNode->aabb.intersects(ray, tmin, tmax))
  {
    return false;
  }
  if (currentNode->model.numIndices > 0)
  {
    return testModel(currentNode->model, ray, tmin, tmax);
  }
  auto leftResults = testIntersection(currentNode->left, ray, tmin, tmax);
  auto rightResults = testIntersection(currentNode->right, ray, tmin, tmax);

  return leftResults || rightResults;
}

std::vector<IntersectionInfo> Scene::generateIntersections(const PNode& currentNode, const Ray ray, const float tmin,
                                                           float tmax) const noexcept
{
  if (!currentNode->aabb.intersects(ray, tmin, tmax))
  {
    return {};
  }
  if (currentNode->model.numIndices > 0)
  {
    return intersectModel(currentNode->model, ray, tmin, tmax);
  }
  auto leftResults = generateIntersections(currentNode->left, ray, tmin, tmax);
  auto rightResults = generateIntersections(currentNode->right, ray, tmin, tmax);

  for (auto& it : rightResults)
  {
    leftResults.push_back(std::move(it));
  }
  return leftResults;
}

bool Scene::testModel(const ModelReference& reference, const Ray ray, const float tmin, float tmax) const noexcept
{
  float distance = 0;

  for (size_t posIndex = 0, edgeIndex = 0, normalIndex = 0; posIndex < reference.numIndices; posIndex++, edgeIndex += 2, normalIndex++)
  {
    const auto i0 = indicesPool[reference.indicesOffset + posIndex].x;
    const auto i1 = indicesPool[reference.indicesOffset + posIndex].y;
    const auto i2 = indicesPool[reference.indicesOffset + posIndex].z;

    const auto& p0 = positionPool[reference.positionOffset + i0];
    const auto& p1 = positionPool[reference.positionOffset + i1];
    const auto& p2 = positionPool[reference.positionOffset + i2];

    const auto& t0 = texCoordsPool[reference.positionOffset + i0];
    const auto& t1 = texCoordsPool[reference.positionOffset + i1];
    const auto& t2 = texCoordsPool[reference.positionOffset + i2];

    const auto& e0 = edgesPool[reference.indicesOffset + edgeIndex];
    const auto& e1 = edgesPool[reference.indicesOffset + edgeIndex + 1];

    const auto& n = faceNormalsPool[reference.indicesOffset + normalIndex];

    const auto s = ray.origin - p0;
    const auto s1 = glm::cross(ray.direction, e1);
    const auto s2 = glm::cross(s, e0);

    const float fraction = 1.0f / glm::dot(s1, e0);
    const auto resultVector = glm::vec3(glm::dot(s2, e1), glm::dot(s1, s), glm::dot(s2, ray.direction)) * fraction;

    const float b3 = 1.0f - resultVector.y - resultVector.z;

    const auto texCoords = t0 * resultVector.y + t1 * resultVector.z + t2 * b3;

    if (b3 < 0 || b3 > 1)
      continue;
    if (resultVector.y < 0 || resultVector.y > 1)
      continue;
    if (resultVector.z < 0 || resultVector.z > 1)
      continue;

    if (resultVector.x < tmin || resultVector.x > tmax)
      continue;
    return true;
  }

  return false;
}
std::vector<IntersectionInfo> Scene::intersectModel(const ModelReference& reference, const Ray ray, const float tmin,
                                                    float tmax) const noexcept
{
  std::vector<IntersectionInfo> intersection = {};
  float distance = 0;

  for (size_t posIndex = 0, edgeIndex = 0, normalIndex = 0; posIndex < reference.numIndices; posIndex++, edgeIndex += 2, normalIndex++)
  {
    const auto i0 = indicesPool[reference.indicesOffset + posIndex].x;
    const auto i1 = indicesPool[reference.indicesOffset + posIndex].y;
    const auto i2 = indicesPool[reference.indicesOffset + posIndex].z;

    const auto& p0 = positionPool[reference.positionOffset + i0];
    const auto& p1 = positionPool[reference.positionOffset + i1];
    const auto& p2 = positionPool[reference.positionOffset + i2];

    const auto& t0 = texCoordsPool[reference.positionOffset + i0];
    const auto& t1 = texCoordsPool[reference.positionOffset + i1];
    const auto& t2 = texCoordsPool[reference.positionOffset + i2];

    const auto& e0 = edgesPool[reference.indicesOffset + edgeIndex];
    const auto& e1 = edgesPool[reference.indicesOffset + edgeIndex + 1];

    const auto& n = faceNormalsPool[reference.indicesOffset + normalIndex];

    const auto s = ray.origin - p0;
    const auto s1 = glm::cross(ray.direction, e1);
    const auto s2 = glm::cross(s, e0);

    const float fraction = 1.0f / glm::dot(s1, e0);
    const auto resultVector = glm::vec3(glm::dot(s2, e1), glm::dot(s1, s), glm::dot(s2, ray.direction)) * fraction;

    const float b3 = 1.0f - resultVector.y - resultVector.z;

    const auto texCoords = t0 * resultVector.y + t1 * resultVector.z + t2 * b3;

    if (b3 < 0 || b3 > 1)
      continue;
    if (resultVector.y < 0 || resultVector.y > 1)
      continue;
    if (resultVector.z < 0 || resultVector.z > 1)
      continue;

    if (resultVector.x < tmin || resultVector.x > tmax)
      continue;

    intersection.push_back(IntersectionInfo{
        .hitInfo =
            {
                .t = resultVector.x,
                .position = ray.origin + ray.direction * resultVector.x,
                .normal = n,
                .normalLight = n, // todo: flip based on something, idk what
                .texCoords = texCoords,
            },
        .brdf =
            {
                .albedo = glm::vec3(texCoords, 0.0f),
                .emissive = glm::vec3(0.0f, 0.0f, 0.0f),
            },
    });
    distance = resultVector.x;
  }

  return intersection;
}
