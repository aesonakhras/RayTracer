#pragma once
#include "glm/glm.hpp"
#include <string>
#include <vector>
#include "Material.h"


struct GlobalData {
	float Shininess;
	int Antialias;
	glm::vec3 bgColor;
	int MaxDepth;
	glm::vec2 Resolution;
};


struct LightData {
	std::string Name;

	glm::vec3 Pos;
	glm::vec3 Diff;
	glm::vec3 Spec;
};

struct AnimMetaData {
	int Fps;
	float Time;
};

struct Instruction {
	std::string Name;
	float StartT;
	float EndT;
	float	MoveType;
	glm::vec3 StartPos;
	glm::vec3 EndPos;

	float LocalTime;
};

struct Ray {
	glm::vec3 dir;
	glm::vec3 origin;
};

struct Hit {
	float t;
	glm::vec3 location;
	glm::vec3 normal;

	//ref to the material of the hit object
	Material* mat;

	Hit() {
		this->normal = glm::vec3(0);
		this->location = glm::vec3(0);
		this->mat = nullptr;
	}
};



