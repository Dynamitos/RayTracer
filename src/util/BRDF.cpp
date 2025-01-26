#include "BRDF.h"
#include "Model.h"

glm::vec3 BRDF::evaluate(HitInfo hit, glm::vec3 viewDir, glm::vec3 lightDir, glm::vec3 lightColor)
{
  // todo: switch for materialtype, this is blinnphong
  glm::vec3 normal = hit.normal;
  float diffuse = std::max(glm::dot(normal, lightDir), 0.0f);
  glm::vec3 h = glm::normalize(lightDir + viewDir);
  float specular = pow(std::min(std::max(glm::dot(normal, h), 0.0f), 1.0f), shininess);

  return (albedo * diffuse * lightColor) + (specularColor * specular);
}
