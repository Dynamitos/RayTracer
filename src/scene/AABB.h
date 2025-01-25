#pragma once
#include <array>
#include <glm/glm.hpp>
#include <algorithm>
#include "util/Ray.h"
#include <iostream>

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
    void transform(glm::mat4 transform)
    {
        std::array<glm::vec3, 8> corners;
        corners[0] = glm::vec3(min.x, min.y, min.z);
        corners[1] = glm::vec3(min.x, min.y, max.z);
        corners[2] = glm::vec3(min.x, max.y, min.z);
        corners[3] = glm::vec3(min.x, max.y, max.z);
        corners[4] = glm::vec3(max.x, min.y, min.z);
        corners[5] = glm::vec3(max.x, min.y, max.z);
        corners[6] = glm::vec3(max.x, max.y, min.z);
        corners[7] = glm::vec3(max.x, max.y, max.z);
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(std::numeric_limits<float>::lowest());
        for (int i = 0; i < 8; ++i)
        {
            glm::vec4 transformed = transform * glm::vec4(corners[i].x, corners[i].y, corners[i].z, 1.0f);
            min = glm::vec3(std::min(min.x, transformed.x), std::min(min.y, transformed.y), std::min(min.z, transformed.z));
            max = glm::vec3(std::max(max.x, transformed.x), std::max(max.y, transformed.y), std::max(max.z, transformed.z));
        }
    }
    bool intersects(Ray ray, float tmin, float tmax) const
    {
        glm::vec3 invD = 1.0f / ray.direction;
        glm::vec3 t0s = glm::vec3(min - ray.origin) * invD;
        glm::vec3 t1s = glm::vec3(max - ray.origin) * invD;

        glm::vec3 tsmaller = glm::min(t0s, t1s);
        glm::vec3 tbigger = glm::max(t0s, t1s);

        tmin = std::max(tmin, std::max(tsmaller.x, std::max(tsmaller.y, tsmaller.z)));
        tmax = std::min(tmax, std::min(tbigger.x, std::min(tbigger.y, tbigger.z)));

        return (tmin < tmax);
    }
    static AABB combine(AABB lhs, AABB rhs)
    {
        AABB result = {
            .min = glm::vec3(std::min(lhs.min.x, rhs.min.x), std::min(lhs.min.y, rhs.min.y), std::min(lhs.min.z, rhs.min.z)),
            .max = glm::vec3(std::max(lhs.max.x, rhs.max.x), std::max(lhs.max.y, rhs.max.y), std::max(lhs.max.z, rhs.max.z)),
        };

        return result;
    }
};
