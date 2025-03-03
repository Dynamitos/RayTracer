#pragma once
#include <glm/glm.hpp>

struct Camera
{
    glm::vec3 position;
    glm::vec3 target;
    glm::vec2 sensorSize = glm::vec2(0.036, 0.024);
    float S_O = 20;
    float f = 0.7;
    float A = 0.35;
};