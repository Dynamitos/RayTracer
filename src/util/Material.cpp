#include "Material.h"

std::unique_ptr<BRDF> Material::evaluate(HitInfo hitInfo)
{
  return std::make_unique<BlinnPhong>();
}
