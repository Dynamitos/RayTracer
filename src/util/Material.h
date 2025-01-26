#pragma once
#include "Minimal.h"
#include "Texture.h"
#include "Ray.h"
#include "BRDF.h"

class Material
{
public:
  BRDF evaluate(HitInfo hitInfo);
  PTexture albedoTexture;
  PTexture emissiveTexture;
};