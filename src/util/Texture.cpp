#include "Texture.h"

glm::vec3 Texture::sample(glm::vec2 texCoords) const
{
  uint32_t x = texCoords.x * width;
  uint32_t y = texCoords.y * height;
  return textureData[y * width + x];
}