#pragma once
#include "Minimal.h"
#include "Material.h"
#include "scene/AABB.h"
#include <optional>
#include <vector>

struct IntersectionInfo
{
  HitInfo hitInfo;
  BRDF brdf;
};
class Model
{
public:
  AABB boundingBox;
  std::vector<glm::vec3> positions;
  std::vector<glm::vec2> texCoords;
  std::vector<glm::vec3> normals;
  std::vector<glm::uvec3> indices;
  std::vector<glm::vec3> edges;
  std::vector<glm::vec3> faceNormals;
  void transform(glm::mat4 matrix);
};
DECLARE_REF(Model)