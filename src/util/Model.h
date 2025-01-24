#pragma once
#include "Minimal.h"
#include "scene/AABB.h"
#include <vector>
#include <optional>

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
	std::vector<uint32_t> indices;
        std::vector<glm::vec3> es;
        std::vector<glm::vec3> faceNormals;
	std::optional<IntersectionInfo> intersect(Ray ray);
};
DECLARE_REF(Model)