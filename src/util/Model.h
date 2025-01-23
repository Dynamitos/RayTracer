#pragma once
#include "Minimal.h"
#include "scene/AABB.h"
#include <vector>

class Model
{
public:
    AABB boundingBox;
	std::vector<glm::vec3> positions;
	std::vector<uint32_t> indices;
};
DECLARE_REF(Model)