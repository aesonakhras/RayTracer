#include <iostream>
#include <string>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/error/en.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/compatibility.hpp"
#include "CustomVec.h"
#include "ExternalFile.h"

#include <omp.h>

#include <chrono>
#include <time.h>


using namespace rapidjson;

enum MatType {
	MIRROR,
	DIFFUSE
};

struct GlobalData {
	float Shininess;
	int Antialias;
	glm::vec3 bgColor;
	int MaxDepth;
	glm::vec2 Resolution;
};

struct Material {
	glm::vec3 Diff;
	glm::vec3 Spec;
	glm::vec3 Amb;

	MatType MatType;
};

struct LightData {
	std::string Name;
	
	glm::vec3 Pos;
	glm::vec3 Diff;
	glm::vec3 Spec;
};

struct Sphere {
	std::string Name;
	glm::vec3 Pos;
	float Radius;

	Material Mat;
};

 struct Quad {
	std::string Name;
	glm::vec3 Pos1;
	glm::vec3 Pos2;
	glm::vec3 Pos3;

	Material Mat;
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

class IObject {

};



struct Cell {
	std::vector<IObject> objects;
};

class Grid {
	
};



std::vector<LightData> g_lights;
std::vector<Sphere> g_spheres;
std::vector<Quad> g_quads;

std::vector<Instruction> g_animInstructions;

GlobalData g_data;
AnimMetaData g_animData;

glm::vec3 g_eye;


float g_aspectRatio;
float g_fieldOfView = 60.0f;

void SetVecFromJson(glm::vec3* vec, std::string name) {

}

float colorRange = 255.f;

void LoadScene() {
	std::string sceneFile = ExternalFile::Load("Scene.json");

	rapidjson::Document d;
	ParseResult result = d.Parse(sceneFile.c_str());
	if (!result) {
		fprintf(stderr, "JSON parse error: %s (%u)",
			GetParseError_En(result.Code()), result.Offset());
		exit(EXIT_FAILURE);
		return;
	}


	rapidjson::Value& a = *GetValueByPointer(d, "/SceneSettings");
	g_data.Shininess = a["SHININESS"].GetFloat();

	g_data.Antialias = a["ANTIALIAS"].GetInt();


	//Try to speed this up
	g_data.bgColor.x = a["BACKGROUND"][0].GetFloat();
	g_data.bgColor.y = a["BACKGROUND"][1].GetFloat();
	g_data.bgColor.z = a["BACKGROUND"][2].GetFloat();

	g_data.MaxDepth = a["MAXDEPTH"].GetInt();

	g_data.Resolution.x = a["RESOLUTION"][0].GetInt();
	g_data.Resolution.y = a["RESOLUTION"][1].GetInt();


	//Get the Lights
	a = *GetValueByPointer(d, "/Lights");
	assert(a.IsArray());

	for (int i = 0; i < a.Size(); i++) {
		LightData light;

		light.Name = a[i]["Name"].GetString();

		//How does this even work???
		//This is pretty cool!
		light.Pos.x = a[i]["Pos"][0].GetFloat();
		light.Pos.y = a[i]["Pos"][1].GetFloat();
		light.Pos.z = a[i]["Pos"][2].GetFloat();

		light.Diff.x = a[i]["Diff"][0].GetFloat() / colorRange;
		light.Diff.y = a[i]["Diff"][1].GetFloat() / colorRange;
		light.Diff.z = a[i]["Diff"][2].GetFloat() / colorRange;

		light.Spec.x = a[i]["Spec"][0].GetFloat() / colorRange;
		light.Spec.y = a[i]["Spec"][1].GetFloat() / colorRange;
		light.Spec.z = a[i]["Spec"][2].GetFloat() / colorRange;

		g_lights.push_back(light);
	}

	//Get Spheres
	a = *GetValueByPointer(d, "/Spheres");
	assert(a.IsArray());
	
	for (int i = 0; i < a.Size(); i++) {
		Sphere sphere;
		sphere.Name = a[i]["Name"].GetString();

		sphere.Pos.x = a[i]["Pos"][0].GetFloat();
		sphere.Pos.y = a[i]["Pos"][1].GetFloat();
		sphere.Pos.z = a[i]["Pos"][2].GetFloat();

		sphere.Radius = a[i]["Radius"].GetFloat();
		
		sphere.Mat.Diff.r = a[i]["Diff"][0].GetFloat() / colorRange;
		sphere.Mat.Diff.g = a[i]["Diff"][1].GetFloat() / colorRange;
		sphere.Mat.Diff.b = a[i]["Diff"][2].GetFloat() / colorRange;

		sphere.Mat.Spec.r = a[i]["Spec"][0].GetFloat() / colorRange;
		sphere.Mat.Spec.g = a[i]["Spec"][1].GetFloat() / colorRange;
		sphere.Mat.Spec.b = a[i]["Spec"][2].GetFloat() / colorRange;

		sphere.Mat.Amb.r = a[i]["Amb"][0].GetFloat() / colorRange;
		sphere.Mat.Amb.g = a[i]["Amb"][1].GetFloat() / colorRange;
		sphere.Mat.Amb.b = a[i]["Amb"][2].GetFloat() / colorRange;

		int matType = a[i]["MatType"].GetInt();

		if (matType == 0) {
			sphere.Mat.MatType = MatType::MIRROR;
		}
		else {
			sphere.Mat.MatType = MatType::DIFFUSE;
		}

		g_spheres.push_back(sphere);
	}

	a = *GetValueByPointer(d, "/Quads");
	assert(a.IsArray());

	for (int i = 0; i < a.Size(); i++) {
		Quad quad;

		quad.Name = a[i]["Name"].GetString();

		quad.Pos1.x = a[i]["Pos1"][0].GetFloat();
		quad.Pos1.y = a[i]["Pos1"][1].GetFloat();
		quad.Pos1.z = a[i]["Pos1"][2].GetFloat();

		quad.Pos2.x = a[i]["Pos2"][0].GetFloat();
		quad.Pos2.y = a[i]["Pos2"][1].GetFloat();
		quad.Pos2.z = a[i]["Pos2"][2].GetFloat();

		quad.Pos3.x = a[i]["Pos3"][0].GetFloat();
		quad.Pos3.y = a[i]["Pos3"][1].GetFloat();
		quad.Pos3.z = a[i]["Pos3"][2].GetFloat();

		quad.Mat.Diff.r = a[i]["Diff"][0].GetFloat() / colorRange;
		quad.Mat.Diff.g = a[i]["Diff"][1].GetFloat() / colorRange;
		quad.Mat.Diff.b = a[i]["Diff"][2].GetFloat() / colorRange;

		quad.Mat.Spec.r = a[i]["Spec"][0].GetFloat() / colorRange;
		quad.Mat.Spec.g = a[i]["Spec"][1].GetFloat() / colorRange;
		quad.Mat.Spec.b = a[i]["Spec"][2].GetFloat() / colorRange;

		quad.Mat.Amb.r = a[i]["Amb"][0].GetFloat() / colorRange;
		quad.Mat.Amb.g = a[i]["Amb"][1].GetFloat() / colorRange;
		quad.Mat.Amb.b = a[i]["Amb"][2].GetFloat() / colorRange;

		int matType = a[i]["MatType"].GetInt();

		if (matType == 0) {
			quad.Mat.MatType = MatType::MIRROR;
		}
		else {
			quad.Mat.MatType = MatType::DIFFUSE;
		}

		g_quads.push_back(quad);
	}

}
int g_framesToRender;
void LoadAnimationDesc() {
	//Set variables that are necessary
	g_framesToRender = 1;

	std::string sceneFile = ExternalFile::Load("Animation.json");

	rapidjson::Document d;
	ParseResult result = d.Parse(sceneFile.c_str());
	if (!result) {
		fprintf(stderr, "JSON parse error: %s (%u)",
			GetParseError_En(result.Code()), result.Offset());
		exit(EXIT_FAILURE);
		return;
	}


	rapidjson::Value& a = *GetValueByPointer(d, "/MetaData");
	g_animData.Fps = a["Fps"].GetInt();
	g_animData.Time = a["Time"].GetFloat();

	a = *GetValueByPointer(d, "/Instructions");
	assert(a.IsArray());

	for (int i = 0; i < a.Size(); i++) {
		Instruction instruction;
		
		
		instruction.Name = a[i]["Name"].GetString();
		instruction.StartT = a[i]["StartT"].GetFloat();
		instruction.EndT = a[i]["EndT"].GetFloat();
		instruction.MoveType = a[i]["MoveType"].GetFloat();

		instruction.StartPos.x = a[i]["StartPos"][0].GetFloat();
		instruction.StartPos.y = a[i]["StartPos"][1].GetFloat();
		instruction.StartPos.z = a[i]["StartPos"][2].GetFloat();

	
		instruction.EndPos.x = a[i]["EndPos"][0].GetFloat();
		instruction.EndPos.y = a[i]["EndPos"][1].GetFloat();
		instruction.EndPos.z = a[i]["EndPos"][2].GetFloat();

		instruction.LocalTime = 0.0f;

		g_animInstructions.push_back(instruction);
	}

	g_framesToRender = g_animData.Fps * g_animData.Time;
}



Ray CalculateRay(int i, int j) {

	//NOTE, I and J are flipped as I would like to process each row instead of column first
	glm::vec3 point = glm::vec3(0, 0, 0);

	float fov = glm::tan(glm::radians(g_fieldOfView) / 2);
	//float fov = g_fieldOfView;

	float fuckx = j - (g_data.Resolution.x / 2.0f);
	float fucky = i - (g_data.Resolution.y / 2.0f);

	point.x = g_aspectRatio  * ((2 * fuckx) / g_data.Resolution.x) * fov;
	point.y =  ((2 * fucky)/ g_data.Resolution.y) * -fov;
	//point.z = -1.0f / glm::tan(glm::radians(g_fieldOfView)/2 );
	point.z = -449;

	Ray ray;
	ray.dir = glm::normalize(point - g_eye);

	ray.origin = g_eye;
	return ray;
}


bool interSectRaySphere(Sphere* sphere, Ray ray, Hit& hit) {
	glm::vec3 v = sphere->Pos - ray.origin;
	float t0 = glm::dot(v, ray.dir);

	float Dsqr = glm::dot(v, v) - (t0 * t0);

	float td = (sphere->Radius * sphere->Radius) - Dsqr;

	if (td < 0) {
		return false;
	}

	//tangent to sphere
	//Find epsilon
	if (td <= 0.0001) {
		hit.t = td;
		return true;
	}

	//two indersection points
	hit.t = std::min(t0 + sqrt(td), t0 - sqrt(td));
	return  true;
}

bool InterSectRayQuad(Quad* quad, Ray ray, Hit& hit) {
	glm::vec3 a = quad->Pos1;
	glm::vec3 b = quad->Pos2;
	glm::vec3 c = quad->Pos3;

	//1
	glm::vec3 p = b - a;
	glm::vec3 q = c - a;

	//2
	glm::vec3 tmp1 = glm::cross(ray.dir, q);

	//3
	float dot1 = glm::dot(tmp1, p);

	//4
	float eps = powf(1, -5);
	if ((dot1 > -eps) && dot1 < eps) return false;

	//5
	float f = 1.0f / dot1;

	//6
	glm::vec3 s = ray.origin - a;

	//7
	float u = f * glm::dot(s, tmp1);

	//8
	if ((u < 0.f) || (u > 1.f)) return false;

	//9
	glm::vec3 tmp2 = glm::cross(s, p);

	//10
	float v = f * glm::dot(ray.dir, tmp2);

	//11
	if ((v < 0.f) || (v > 1.f)) return false;
	//if ((v < 0.f) || (u + v > 1.f)) return 0;

	//12
	hit.t = f * glm::dot(q, tmp2);

	hit.normal = glm::normalize(glm::cross(q, p));

	return true;
}

//Returns a properly filled in hit if there is an intersection
//Otherwise it returns false and hit is undefined
bool FirstIntersection(Ray& ray, Hit& hit) {

	hit.t = std::numeric_limits<float>::max();

	//Check sphere
	for (int i = 0; i < g_quads.size(); i++) {
		Hit tempHit;
		if (InterSectRayQuad(&g_quads[i], ray, tempHit)) {
			if (tempHit.t < hit.t && tempHit.t >= 0.01) {
				hit.mat = &g_quads[i].Mat;
				hit.t = tempHit.t;

				hit.normal = tempHit.normal;
			}
		}
	}

	for (int i = 0; i < g_spheres.size(); i++) {
		Hit tempHit;
		if ( interSectRaySphere( &g_spheres[i], ray, tempHit)) {
			if (tempHit.t < hit.t && tempHit.t >= 0.01) {
				hit.mat = &g_spheres[i].Mat;
				hit.t = tempHit.t;
				//Can optimize here
				hit.location = ray.origin + (hit.t * ray.dir);
				hit.normal = glm::normalize(hit.location - g_spheres[i].Pos);
			}
		}
	}

	//calculate the intersection point here
	hit.location = ray.origin + (hit.t * ray.dir);
	//Check if the material has been set, if so we have a hit
	return !(hit.mat == nullptr);
}

glm::vec3 CalculateSingleLight(Hit hit, LightData light, Ray ray) {
	
	 //result = glm::vec3(0);

	glm::vec3 lightVec = glm::normalize(light.Pos - hit.location);
	
	//Diffuse
	glm::vec3 result = light.Diff * hit.mat->Diff * glm::clamp(glm::dot(lightVec, hit.normal), 0.0f, 1.0f);

	glm::vec3 view = glm::normalize(hit.location - ray.origin);

	//Calculate the reflected vector
	glm::vec3 reflected = glm::normalize(glm::reflect(lightVec, hit.normal));

	//Specular
	result += light.Spec * hit.mat->Spec * pow(glm::clamp(dot(view, reflected ), 0.0f, 1.0f), g_data.Shininess);
	//result = hit.normal * 2.0f - 1.0f;
	return result * 0.45f;
}

glm::vec3 CalculateLighting(Ray ray, Hit hit) {
	glm::vec3 color = glm::vec3(0);


	for (int i = 0; i < g_lights.size(); i++) {
		color += CalculateSingleLight(hit, g_lights[i], ray);
	}

	for (int i = 0; i < g_lights.size(); i++) {

		Ray shadowRay;
		shadowRay.origin = hit.location;

		glm::vec3 lightVec = g_lights[i].Pos - hit.location;
		shadowRay.dir = glm::normalize(g_lights[i].Pos - hit.location);

		Hit shadowHit = Hit();
		
		if (FirstIntersection(shadowRay, shadowHit)) {
			//Check distance between light and hit
			float lightVecMag = glm::dot(lightVec, lightVec);

			glm::vec3 hitVec = shadowHit.location - hit.location;
			float hitvecMag = glm::dot(hitVec, hitVec);

			if (hitvecMag >= lightVecMag) {
				color += CalculateSingleLight(hit, g_lights[i], ray);
			}
		}
		else {
			color += CalculateSingleLight(hit, g_lights[i], ray);
		}
	}

	//color = glm::clamp(color, 0.0f, 1.0f);

	return color;
}



glm::vec3 TraceRay(Ray ray, int maxDepth) {

	

	Hit hit = Hit();
	bool p = FirstIntersection(ray, hit); //get the first one
	if (!p) return glm::vec3(0);//out of scene


	//Calculate the local lighting
	glm::vec3 color = CalculateLighting(ray, hit);

	if (maxDepth > 0 && hit.mat->MatType == MatType::MIRROR) {
		Ray newRay;
		newRay.origin = hit.location;
		newRay.dir = glm::reflect(ray.dir, hit.normal);

		color += TraceRay(newRay, maxDepth - 1);
	}

	color = glm::clamp(color, 0.0f, 1.0f);


	return color;
}

float g_currentTime;

void SetUpNextFrame() {
	g_currentTime += 1.0f/(float)g_animData.Fps;

	//Loop through all instructions to determine where to move each geometry
	for (int i = 0; i < g_animInstructions.size(); i++) {
		//Find which object we are trying to move
		for (int j = 0; j < g_spheres.size(); j++) {
			if (g_spheres[j].Name.compare(g_animInstructions[i].Name) == 0) {
				//Check if this animation is active
				Instruction* instruction = &g_animInstructions[i];
				if ((g_currentTime >=  instruction->StartT) && (g_currentTime <= instruction->EndT + 0.0001f)) {
					instruction->LocalTime += 1.0f / (float)g_animData.Fps;
					g_spheres[i].Pos = glm::lerp(instruction->StartPos, instruction->EndPos, instruction->LocalTime);
				}
			}
			
		}
	}
}

bool singleFrame = false;

std::chrono::high_resolution_clock::time_point start;
int main(int* argc, char** argv) {
	int frameCount = 0;
	
	start = std::chrono::high_resolution_clock::now();
	g_eye = glm::vec3(0, 0, -450);

	
	LoadScene();
	LoadAnimationDesc();

	g_aspectRatio = g_data.Resolution.x / g_data.Resolution.y;

	bool doneRendering = false;

	float totalTimeRendering = 0.0f;
	int framesRendererd = 0;
	//incrument to start not taken affect until second frame
	do {
		ExternalFile newFile = ExternalFile();


		newFile.BMPInit(g_data.Resolution.x, g_data.Resolution.y);

		#pragma omp parallel for
		for (int i = 0; i < (int)g_data.Resolution.y; i++) {
			for (int j = 0; j < (int)g_data.Resolution.x; j++) {

				Ray ray = CalculateRay(i, j); //get the primary ray
				glm::vec3 color = TraceRay(ray, g_data.MaxDepth);
				CV::ColorBMP realColor = CV::ColorBMP(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f);
				//CV::ColorBMP color = CV::ColorBMP(((ray.x * 0.5 + 0.5) * 255), ((ray.y * 0.5 + 0.5) * 255),((ray.z * 0.5 + 0.5) * 255)) ;
				newFile.BMPBuffer(i, j, realColor);
			}
		}

		std::string fileName = "OutPut" + std::to_string(frameCount) + ".bmp";
		newFile.BMPWrite(fileName);
		frameCount++;

		double end = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

		totalTimeRendering += end / 10000000.0;
		std::cout << "Frame Completed in: " << end / 10000000.0 << " Seconds" << std::endl;
		start = std::chrono::high_resolution_clock::now();

		
		
		g_framesToRender--;
		
		//How to avoid double Check
		if (g_framesToRender >= 0) {
			SetUpNextFrame();
		}
		framesRendererd++;
	} while (g_framesToRender >= 0);
	std::cout << "Frame Avg: " << totalTimeRendering / (float)framesRendererd << " Seconds" << std::endl;

	return 0;
}