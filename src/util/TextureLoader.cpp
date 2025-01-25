#include "TextureLoader.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

PTexture TextureLoader::loadTexture(std::string_view filename)
{
  int x, y, n;
  auto* data = stbi_load(filename.data(), &x, &y, &n, 3);
  std::vector<glm::vec3> texData(x * y);
  for (uint32_t i = 0; i < texData.size(); ++i)
  {
    texData[i] = glm::vec3(data[i * 3 + 0] / 256.f, data[i * 3 + 1] / 256.f, data[i * 3 + 2] / 256.f);
  }
  auto result = std::make_unique<Texture>();
  result->textureData = std::move(texData);
  result->width = x;
  result->height = y;
  return result;
}