#pragma once

#include "IGeometry.h"

class Sphere : public IGeometry {
public:

	Sphere(glm::vec3 pos, float radius, 
						glm::vec3 Diff,
						glm::vec3 Spec,
						glm::vec3 Amb,

						MatType matType,
						std::string name);
	~Sphere();

	glm::vec3 m_pos;
	float m_radius;

	inline virtual bool CalculateIntersection(Ray ray, Hit& hit) override;
};