#pragma once
#include "AABB.h"
#include "util/Model.h"
#include "util/Ray.h"
#include <glm/glm.hpp>
#include <optional>
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
  void addPointLight(PointLight point) { pointLights.push_back(point); }
  void addDirectionalLight(DirectionalLight dir) { directionalLights.push_back(dir); }
  void addModel(PModel model, glm::mat4 transform);
  void addModels(std::vector<PModel> models, glm::mat4 transform);
  virtual void generate();

  void traceRay(Ray ray, Payload& payload, const float tmin, const float tmax) const noexcept;

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

  DECLARE_REF(Node)
  struct Node
  {
    PNode left;
    PNode right;
    AABB aabb;
    ModelReference model;
    Node(AABB aabb) : aabb(aabb) {}
    Node(AABB aabb, ModelReference model) : aabb(aabb), model(model) {}
  };
  PNode hierarchy;
  std::vector<PModel> models;

  void populateGeometryPools();

  // tests if a ray intersects any geometry, no hit information, for shadow rays
  bool testIntersection(const PNode& currentNode, const Ray ray, const float tmin, const float tmax) const noexcept;
  IntersectionInfo generateIntersections(const PNode& currentNode, const Ray ray, const float tmin, const float tmax) const noexcept;
  bool testModel(const ModelReference& reference, const Ray ray, const float tmin, const float tmax) const noexcept;
  IntersectionInfo intersectModel(const ModelReference& reference, const Ray ray, const float tmin, const float tmax) const noexcept;
};