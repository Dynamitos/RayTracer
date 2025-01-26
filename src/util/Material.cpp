#include "Material.h"
#include "BRDF.h"

BRDF Material::evaluate(HitInfo hitInfo)
{
  return BRDF{
      .albedo = glm::vec3(hitInfo.texCoords, 0),
      .alpha = 1,
      .emissive = glm::vec3(0, 0, 0),
      .materialType = MaterialType::BlinnPhong,
  };
}
