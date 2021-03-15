#include "Quad.h"

Quad::Quad(glm::vec3 Pos1,
	glm::vec3 Pos2,
	glm::vec3 Pos3,

	glm::vec3 Diff,
	glm::vec3 Spec,
	glm::vec3 Amb,

	MatType matType,
	std::string name) :
	m_Pos1(Pos1),
	m_Pos2(Pos2),
	m_Pos3(Pos3),
	IGeometry(Diff, Spec, Amb, matType, name)
{
	glm::vec3 a = m_Pos1;
	glm::vec3 b = m_Pos2;
	glm::vec3 c = m_Pos3;

	//1
	m_p = b - a;
	m_q = c - a;
	m_normal = glm::normalize(glm::cross(m_q, m_p));
}

Quad::~Quad() {

}


inline bool Quad::CalculateIntersection(Ray ray, Hit& hit) {


	//2
	glm::vec3 tmp1 = glm::cross(ray.dir, m_q);

	//3
	float dot1 = glm::dot(tmp1, m_p);

	//4
	float eps = powf(1, -5);
	if ((dot1 > -eps) && dot1 < eps) return false;

	//5
	float f = 1.0f / dot1;

	//6
	glm::vec3 s = ray.origin - m_Pos1;

	//7
	float u = f * glm::dot(s, tmp1);

	//8
	if ((u < 0.f) || (u > 1.f)) return false;

	//9
	glm::vec3 tmp2 = glm::cross(s, m_p);

	//10
	float v = f * glm::dot(ray.dir, tmp2);

	//11
	if ((v < 0.f) || (v > 1.f)) return false;

	//12
	hit.t = f * glm::dot(m_q, tmp2);
	hit.location = ray.origin + (hit.t * ray.dir);
	hit.normal = m_normal;

	return true;
}


