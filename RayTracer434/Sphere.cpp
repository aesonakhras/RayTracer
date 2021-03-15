#include "Sphere.h"

Sphere::Sphere(glm::vec3 pos, float radius,
	glm::vec3 Diff,
	glm::vec3 Spec,
	glm::vec3 Amb,

	MatType matType,
	std::string name) :
m_pos(pos),
m_radius(radius),
IGeometry(Diff, Spec, Amb, matType, name) { }

Sphere::~Sphere() {

}



inline bool Sphere::CalculateIntersection(Ray ray, Hit& hit) {
	glm::vec3 v = m_pos - ray.origin;
	float t0 = glm::dot(v, ray.dir);

	float Dsqr = glm::dot(v, v) - (t0 * t0);

	float td = (m_radius * m_radius) - Dsqr;

	if (td < 0) {
		return false;
	}

	//tangent to sphere
	//Find epsilon
	if (td <= 0.0001) {
		hit.t = td;

		hit.location = ray.origin + (hit.t * ray.dir);
		hit.normal = glm::normalize(hit.location - m_pos);
		return true;
	}

	//two intersection points
	hit.t = std::min(t0 + sqrt(td), t0 - sqrt(td));

	hit.location = ray.origin + (hit.t * ray.dir);
	hit.normal = glm::normalize(hit.location - m_pos);

	return true;
}