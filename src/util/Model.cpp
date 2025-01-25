#include "Model.h"

void Model::transform(glm::mat4 matrix)
{
  for (auto& pos : positions)
  {
    pos = glm::vec3(matrix * glm::vec4(pos, 1));
  }

  boundingBox.transform(matrix);

  for (int i = 0; i < indices.size(); i+=3)
  {
    auto e0 = positions[indices[i + 1]] - positions[indices[i + 0]];
    auto e1 = positions[indices[i + 2]] - positions[indices[i + 0]];
    es.push_back(e0);
    es.push_back(e1);

    faceNormals.push_back(glm::cross(e0, e1));
  }
}

std::optional<IntersectionInfo> Model::intersect(const Ray ray) const
{
  std::optional<IntersectionInfo> intersection = {};
  float distance = 0;

  for(size_t posIndex=0, eIndex=0, normalIndex=0;  posIndex <indices.size(); posIndex+=3, eIndex+=2, normalIndex++)
  {
    const auto p0 = positions[indices[posIndex]];
    const auto p1 = positions[indices[posIndex+1]];
    const auto p2 = positions[indices[posIndex+2]];

    const auto e0 = es[eIndex];
    const auto e1 = es[eIndex +1];

    const auto n = faceNormals[normalIndex];

    const auto s = ray.origin - p0;
    const auto s1 = glm::cross(ray.direction, e1);
    const auto s2 = glm::cross(s, e0);

    const float fraction = 1.0f / glm::dot(s1, e0);
    const auto resultVector = glm::vec3(glm::dot(s2, e1), glm::dot(s1, s), glm::dot(s2, ray.direction)) * fraction;

    const double b3 = 1 - resultVector.y - resultVector.z;

    if(b3 < 0 || b3 > 1)
      continue;
    if(resultVector.y < 0 || resultVector.y > 1)
      continue;
    if(resultVector.z < 0 || resultVector.z > 1)
      continue;

    if(std::abs(1.0f - (resultVector.y + resultVector.z + b3)) > 1e-7)
      continue;
    if(resultVector.x < 1e-6)
      continue;

    if(!intersection.has_value() || resultVector.x < distance)
    {
      intersection = IntersectionInfo {
        .position = ray.origin + ray.direction * resultVector.x,
        .normal = n,
        .albedo = glm::vec3(0.7f, 0.7f, 0.7f),
        .emissive = glm::vec3(0.0f, 0.0f, 0.0f)
      };
      distance = resultVector.x;
    }
  }

  return intersection;
}

