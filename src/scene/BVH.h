#pragma once
#include <glm/glm.hpp>
#include <vector>

struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

class BVH
{
public:
private:
	std::vector<AABB> boundingBoxes;
};