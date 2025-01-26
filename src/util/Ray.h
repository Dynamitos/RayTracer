#pragma once
#include <glm/glm.hpp>

struct Payload
{
  glm::vec3 accumulatedRadiance = glm::vec3(0);
  glm::vec3 accumulatedMaterial = glm::vec3(0);
};

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;
    uint32_t depth = 0;
    Payload payload;
};

struct HitInfo
{
  float t;
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoords;
};