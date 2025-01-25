#pragma once
#include <string_view>
#include "Texture.h"

class TextureLoader
{
public:
  static PTexture loadTexture(std::string_view filename);

private:
};