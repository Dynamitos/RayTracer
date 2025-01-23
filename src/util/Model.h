#pragma once
#include "Minimal.h"
#include <vector>
#include <glm/glm.hpp>

class Model
{
public:
	std::vector<glm::vec3> positions;
	std::vector<uint32_t> indices;
};
DECLARE_REF(Model)