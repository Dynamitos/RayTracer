#pragma once
#include <glm/glm.hpp>

enum class MaterialType
{
  BlinnPhong
};

struct BRDF
{
  glm::vec3 albedo = glm::vec3(1, 1, 1);
  float alpha = 1;
  glm::vec3 specularColor = glm::vec3(1, 1, 1);
  float shininess = 0;
  glm::vec3 emissive = glm::vec3(0, 0, 0);
  MaterialType materialType;
  glm::vec3 evaluate(struct HitInfo hit, glm::vec3 viewDir, glm::vec3 lightDir, glm::vec3 lightColor);
};