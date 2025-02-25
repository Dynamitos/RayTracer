#pragma once
#include "scene/Scene.h"

class CPUScene : public Scene {
public:
    CPUScene(){}
    virtual ~CPUScene(){}
    void traceRay(Ray ray, Payload& payload, const float tmin, const float tmax) const noexcept;
    virtual void createRayTracingHierarchy() override;
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
    // tests if a ray intersects any geometry, no hit information, for shadow rays
    bool testIntersection(const PNode& currentNode, const Ray ray, const float tmin, const float tmax) const noexcept;
    IntersectionInfo generateIntersections(const PNode& currentNode, const Ray ray, const float tmin, const float tmax) const noexcept;
    bool testModel(const ModelReference& reference, const Ray ray, const float tmin, const float tmax) const noexcept;
    IntersectionInfo intersectModel(const ModelReference& reference, const Ray ray, const float tmin, const float tmax) const noexcept;
};