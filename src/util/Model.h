#pragma once
#include "Minimal.h"
#include "scene/AABB.h"
#include <optional>
#include <vector>

// material infos
// shading parameter
// hit data
struct IntersectionInfo
{
  float t;
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 albedo;
  glm::vec3 emissive;
  //....
};
class Model
{
public:
  AABB boundingBox;
  std::vector<glm::vec3> positions;
  std::vector<glm::uvec3> indices;
  std::vector<glm::vec3> edges;
  std::vector<glm::vec3> faceNormals;
  void transform(glm::mat4 matrix);
};
DECLARE_REF(Model)