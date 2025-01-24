#include "BVH.h"
#include <algorithm>
#include <ranges>

void BVH::addModel(PModel model, glm::mat4 transform)
{
    model->boundingBox.transform(transform);
    for (auto& point : model->positions)
    {
        point = glm::vec3(transform * glm::vec4(point, 1));
    }
    models.push_back(std::move(model));
}

void BVH::addModels(std::vector<PModel> _models, glm::mat4 transform)
{
    for (int i = 0; i < _models.size(); ++i)
    {
        _models[i]->boundingBox.transform(transform);
        for (auto& point : _models[i]->positions)
        {
            point = glm::vec3(transform * glm::vec4(point, 1));
        }
        models.push_back(std::move(_models[i]));
    }
}

void BVH::generate()
{
    std::vector<PNode> pendingNodes;
    while (!models.empty())
    {
        pendingNodes.push_back(std::make_unique<Node>(std::move(models.back())));
        models.pop_back();
    }
    while (pendingNodes.size() > 1)
    {
        int lhs = pendingNodes.size();
        int rhs = pendingNodes.size();
        float minSurface = std::numeric_limits<float>::max();
        for (int i = 0; i < pendingNodes.size(); ++i)
        {
            for (int j = 0; j < pendingNodes.size(); ++j)
            {
                if (i == j)
                    continue;
                AABB combined = AABB::combine(pendingNodes[i]->aabb, pendingNodes[j]->aabb);
                float surface = combined.surfaceArea();
                if (minSurface > surface)
                {
                    lhs = i;
                    rhs = j;
                    minSurface = surface;
                }
            }
        }
        PNode newNode = std::make_unique<Node>(AABB::combine(pendingNodes[lhs]->aabb, pendingNodes[rhs]->aabb));
        newNode->left = std::move(pendingNodes[lhs]);
        newNode->right = std::move(pendingNodes[rhs]);
        pendingNodes.erase(pendingNodes.begin() + lhs);
        pendingNodes.erase(pendingNodes.begin() + rhs);
        pendingNodes.push_back(std::move(newNode));
    }
    hierarchy = std::move(pendingNodes[0]);
}

std::optional<IntersectionInfo> BVH::traceRay(Ray ray)
{
    auto results = generateIntersections(hierarchy, ray);
    float closestT = std::numeric_limits<float>::max();
    IntersectionInfo info;
    for (uint32_t i = 0; i < results.size(); ++i)
    {
        if (results[i].t < closestT)
        {
            closestT = results[i].t;
            info = results[i];
        }
    }
    if (closestT < std::numeric_limits<float>::max())
    {
        return info;
    }
    return {};
}

std::vector<IntersectionInfo> BVH::generateIntersections(PNode& currentNode, Ray ray)
{
    if (!currentNode->aabb.intersects(ray, 0, 1))
    {
        return {};
    }
    if (currentNode->model != nullptr)
    {
        auto result = currentNode->model->intersect(ray);
        if (result.has_value())
        {
            return {*result};
        }
        else
        {
            return {};
        }
    }
    auto leftResults = generateIntersections(currentNode->left, ray);
    auto rightResults = generateIntersections(currentNode->right, ray);

    for (auto& it : rightResults)
    {
        leftResults.push_back(std::move(it));
    }
    return leftResults;
}