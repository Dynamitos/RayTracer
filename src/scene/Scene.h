#pragma once
#include "util/Model.h"
#include <glm/glm.hpp>
#include <vector>

struct ModelReference
{
  uint32_t positionOffset = 0;
  uint32_t numPositions = 0;
  uint32_t indicesOffset = 0;
  uint32_t numIndices = 0;
};

struct PointLight
{
  glm::vec3 position = glm::vec3(0, 0, 0);
  float pad;
  glm::vec3 color = glm::vec3(1, 1, 1);
  float attenuation = 1;
};

struct DirectionalLight
{
  glm::vec3 direction = glm::vec3(0, 1, 0);
  float pad;
  glm::vec3 color = glm::vec3(1, 1, 1);
  float pad1;
};

class Scene
{
public:
  Scene(){}
  virtual ~Scene(){}
  void addPointLight(PointLight point) { pointLights.push_back(point); }
  void addDirectionalLight(DirectionalLight dir) { directionalLights.push_back(dir); }
  void addModel(PModel model, glm::mat4 transform);
  void addModels(std::vector<PModel> models, glm::mat4 transform);
  void generate();

  constexpr uint32_t getNumDirLights() const { return (uint)directionalLights.size(); }
  constexpr uint32_t getNumPointLights() const { return (uint)pointLights.size(); }

protected:
  std::vector<ModelReference> refs;
  std::vector<glm::vec3> positionPool;
  std::vector<glm::vec2> texCoordsPool;
  std::vector<glm::vec3> normalsPool;
  std::vector<glm::uvec3> indicesPool;
  std::vector<glm::vec3> edgesPool;
  std::vector<glm::vec3> faceNormalsPool;

  std::vector<PointLight> pointLights;
  std::vector<DirectionalLight> directionalLights;

  std::vector<PModel> models;

  virtual void createRayTracingHierarchy() = 0;

  friend class GPURenderer;
};