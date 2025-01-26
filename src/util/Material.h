#pragma once
#include "Minimal.h"
#include "Texture.h"
#include "Ray.h"
#include "BRDF.h"

enum class MaterialType
{
	DIFFUSE,
	SPECULAR,
	REFRACTIVE,
};

class Material
{
public:
  std::unique_ptr<BRDF> evaluate(HitInfo hitInfo);
  PTexture albedoTexture;
  PTexture emissiveTexture;
};