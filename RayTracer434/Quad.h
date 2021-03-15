#pragma once


#include "IGeometry.h"

class Quad : public IGeometry {
	public:
		Quad(glm::vec3 m_Pos1,
			glm::vec3 m_Pos2,
			glm::vec3 m_Pos3,
			
			glm::vec3 Diff,
			glm::vec3 Spec,
			glm::vec3 Amb,

			MatType matType,
			std::string name);

		~Quad();

		glm::vec3 m_Pos1;
		glm::vec3 m_Pos2;
		glm::vec3 m_Pos3;

		inline virtual bool CalculateIntersection(Ray ray, Hit& hit) override;

private:
	glm::vec3 m_p;
	glm::vec3 m_q;
	glm::vec3 m_normal;
};