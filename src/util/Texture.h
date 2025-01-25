#pragma once
#include "Minimal.h"
#include <glm/glm.hpp>
#include <ktx.h>
#include <vector>

class Texture
{
public:
  glm::vec3 sample(glm::vec2 texCoords) const;
  std::vector<glm::vec3> textureData;
  int width;
  int height;
};
DECLARE_REF(Texture)