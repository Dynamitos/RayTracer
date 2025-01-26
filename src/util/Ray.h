#pragma once
#include <glm/glm.hpp>

struct Payload
{
  glm::vec3 accumulatedRadiance = glm::vec3(0);
  glm::vec3 accumulatedMaterial = glm::vec3(1);
  uint32_t depth = 0;
  float emissive = 1;
};

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;
};

struct HitInfo
{
  float t = std::numeric_limits<float>::max();
  glm::vec3 position;
  glm::vec3 normal;
  // not entirely sure what that does
  // its the normal being flipped based on some dot product
  glm::vec3 normalLight;
  glm::vec2 texCoords;
};