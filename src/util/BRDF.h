#pragma once
#include "Material.h"

class BRDF
{
  virtual glm::vec3 evaluate(HitInfo hit, glm::vec3 viewDir, glm::vec3 lightDir, glm::vec3 lightColor) = 0;
};

class BlinnPhong : public BRDF
{
  glm::vec3 albedo = glm::vec3(1, 1, 1);
  float alpha = 1;
  glm::vec3 specularColor = glm::vec3(1, 1, 1);
  float shininess = 0;
  glm::vec3 emissive = glm::vec3(0, 0, 0);
  virtual glm::vec3 evaluate(HitInfo hit, glm::vec3 viewDir, glm::vec3 lightDir, glm::vec3 lightColor);
};