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
  uint32_t indicesOffset = 0;
  uint32_t numIndices = 0;
};

struct PointLight
{
  glm::vec3 position;
  glm::vec3 color;
  float attenuation;
};

struct DirectionalLight
{
  glm::vec3 direction;
  glm::vec3 color;
};

class Scene
{
public:
  void addPointLight(PointLight point) { points.push_back(point); }
  void addDirectionalLight(DirectionalLight dir) { directionalLights.push_back(dir); }
  void addModel(PModel model, glm::mat4 transform);
  void addModels(std::vector<PModel> models, glm::mat4 transform);
  void generate();

  std::optional<IntersectionInfo> traceRay(Ray ray) const;

private:
  std::vector<glm::vec3> positionPool;
  std::vector<glm::vec2> texCoordsPool;
  std::vector<glm::uvec3> indicesPool;
  std::vector<glm::vec3> edgesPool;
  std::vector<glm::vec3> faceNormalsPool;

  std::vector<PointLight> points;
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

  std::vector<IntersectionInfo> generateIntersections(const PNode& currentNode, Ray ray) const;
  std::optional<IntersectionInfo> intersectModel(const ModelReference& reference, Ray ray) const;
};