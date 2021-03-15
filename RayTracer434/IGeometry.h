#pragma once
#include "glm/glm.hpp"
#include <string>
#include <vector>
#include "DataStructures.h"

class IGeometry {
public:
	IGeometry(glm::vec3 Diff,
			  glm::vec3 Spec,
		      glm::vec3 Amb,

		      MatType matType,
			  std::string name)
	{
		m_Material.Diff = Diff;
		m_Material.Spec = Spec;
		m_Material.Amb = Amb;

		m_Material.MatType = matType;
		m_Name = name;
		m_ShadowCache = nullptr;
	};

	~IGeometry() {};

	std::string m_Name;
	Material m_Material;
	IGeometry* m_ShadowCache;

	inline virtual bool CalculateIntersection(Ray ray, Hit& hit) = 0;
};