#pragma once
#include "AABB.h"
#include "util/Model.h"
#include <vector>
#include <glm/glm.hpp>
#include "util/Ray.h"
#include <optional>


class BVH
{
  public:
    void addModel(PModel model, glm::mat4 transform);
    void addModels(std::vector<PModel> models, glm::mat4 transform);
    void generate();

    std::optional<IntersectionInfo> traceRay(Ray ray);

  private:
    DECLARE_REF(Node)
    struct Node
    {
        PNode left;
        PNode right;
        AABB aabb;
        PModel model;
        Node(AABB aabb) : aabb(aabb) {}
        Node(PModel model) : aabb(model->boundingBox), model(std::move(model)) {}
    };
    PNode hierarchy;
    std::vector<PModel> models;

    std::vector<IntersectionInfo> generateIntersections(PNode& currentNode, Ray ray);
};