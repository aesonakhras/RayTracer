#pragma once
#include "glm/glm.hpp"

enum class MatType {
	MIRROR,
	DIFFUSE
};


struct Material {
	glm::vec3 Diff;
	glm::vec3 Spec;
	glm::vec3 Amb;

	MatType MatType;
};
