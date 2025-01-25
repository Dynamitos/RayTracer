#pragma once
#include "AABB.h"
#include "util/Model.h"
#include "util/Ray.h"
#include <glm/glm.hpp>
#include <optional>
#include <vector>

struct ModelReference
{
  uint32_t positionOffset;
  uint32_t indicesOffset;
  uint32_t numIndices;
};

class BVH
{
public:
  void addModel(PModel model, glm::mat4 transform);
  void addModels(std::vector<PModel> models, glm::mat4 transform);
  void generate();

  std::optional<IntersectionInfo> traceRay(Ray ray) const;

private:
  std::vector<glm::vec3> positionPool;
  std::vector<glm::uvec3> indicesPool;
  std::vector<glm::vec3> edgesPool;
  std::vector<glm::vec3> faceNormalsPool;

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
  std::optional<IntersectionInfo> intersectModel(ModelReference reference, Ray ray) const;
};