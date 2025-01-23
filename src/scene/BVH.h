#pragma once
#include "AABB.h"
#include "util/Model.h"
#include <vector>

class BVH
{
  public:
    void addModel(PModel model);
    void addModels(std::vector<PModel> models);
    void generate();

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
};