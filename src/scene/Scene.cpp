#include "Scene.h"
#include <algorithm>
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
      edgesPool.push_back(model->edges[i]);
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
    pendingNodes.erase(pendingNodes.begin() + lhs);
    pendingNodes.erase(pendingNodes.begin() + rhs);
    pendingNodes.push_back(std::move(newNode));
  }
  hierarchy = std::move(pendingNodes[0]);
}

std::optional<IntersectionInfo> Scene::traceRay(Ray ray) const
{
  auto results = generateIntersections(hierarchy, ray);
  float closestT = std::numeric_limits<float>::max();
  IntersectionInfo info;
  for (uint32_t i = 0; i < results.size(); ++i)
  {
    if (results[i].t < closestT)
    {
      closestT = results[i].t;
      info = results[i];
    }
  }
  if (closestT < std::numeric_limits<float>::max())
  {
    return info;
  }
  return {};
}

std::vector<IntersectionInfo> Scene::generateIntersections(const PNode& currentNode, Ray ray) const
{
  if (!currentNode->aabb.intersects(ray, 0, std::numeric_limits<float>::max()))
  {
    return {};
  }
  if (currentNode->model.numIndices > 0)
  {
    auto result = intersectModel(currentNode->model, ray);
    if (result.has_value())
    {
      return {*result};
    }
    else
    {
      return {};
    }
  }
  auto leftResults = generateIntersections(currentNode->left, ray);
  auto rightResults = generateIntersections(currentNode->right, ray);

  for (auto& it : rightResults)
  {
    leftResults.push_back(std::move(it));
  }
  return leftResults;
}

std::optional<IntersectionInfo> Scene::intersectModel(const ModelReference& reference, const Ray ray) const
{
  std::optional<IntersectionInfo> intersection = {};
  float distance = 0;

  for (size_t posIndex = 0, edgeIndex = 0, normalIndex = 0; posIndex < reference.numIndices; posIndex++, edgeIndex += 2, normalIndex++)
  {
    const auto p0 = positionPool[reference.positionOffset + indicesPool[reference.indicesOffset + posIndex].x];
    const auto p1 = positionPool[reference.positionOffset + indicesPool[reference.indicesOffset + posIndex].y];
    const auto p2 = positionPool[reference.positionOffset + indicesPool[reference.indicesOffset + posIndex].z];

    const auto e0 = edgesPool[reference.indicesOffset + edgeIndex];
    const auto e1 = edgesPool[reference.indicesOffset + edgeIndex + 1];

    const auto n = faceNormalsPool[reference.indicesOffset + normalIndex];

    const auto s = ray.origin - p0;
    const auto s1 = glm::cross(ray.direction, e1);
    const auto s2 = glm::cross(s, e0);

    const float fraction = 1.0f / glm::dot(s1, e0);
    const auto resultVector = glm::vec3(glm::dot(s2, e1), glm::dot(s1, s), glm::dot(s2, ray.direction)) * fraction;

    const float b3 = 1.0f - resultVector.y - resultVector.z;

    if (b3 < 0 || b3 > 1)
      continue;
    if (resultVector.y < 0 || resultVector.y > 1)
      continue;
    if (resultVector.z < 0 || resultVector.z > 1)
      continue;

    if (resultVector.x < 1e-6)
      continue;

    if (!intersection.has_value() || resultVector.x < distance)
    {
      intersection = IntersectionInfo{.position = ray.origin + ray.direction * resultVector.x,
                                      .normal = n,
                                      .albedo = glm::vec3(0.7f, 0.7f, 0.7f),
                                      .emissive = glm::vec3(0.0f, 0.0f, 0.0f),};
      distance = resultVector.x;
    }
  }

  return intersection;
}
