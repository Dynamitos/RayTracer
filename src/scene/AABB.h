#pragma once
#include <glm/vec3.hpp>
#include <utility>

struct AABB
{
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
    float surfaceArea() const
    {
        glm::vec3 d = glm::vec3(max.x - min.x, max.y - min.y, max.z - min.z);
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }
    void adjust(glm::vec3 pos)
    {
        min.x = std::min(min.x, pos.x);
        min.y = std::min(min.y, pos.y);
        min.z = std::min(min.z, pos.z);

        max.x = std::max(max.x, pos.x);
        max.y = std::max(max.y, pos.y);
        max.z = std::max(max.z, pos.z);
    }
    static AABB combine(AABB lhs, AABB rhs)
    {
        AABB result;

        result.min.x = std::min(lhs.min.x, rhs.min.x);
        result.min.y = std::min(lhs.min.y, rhs.min.y);
        result.min.z = std::min(lhs.min.z, rhs.min.z);

        result.max.x = std::max(lhs.max.x, rhs.max.x);
        result.max.y = std::max(lhs.max.y, rhs.max.y);
        result.max.z = std::max(lhs.max.z, rhs.max.z);

        return result;
    }
};
