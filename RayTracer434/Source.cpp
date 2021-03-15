#include <iostream>
#include <string>
#include <vector>
#include <omp.h>
#include <chrono>
#include <time.h>

#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/error/en.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/compatibility.hpp"


#include "CustomVec.h"
#include "ExternalFile.h"
#include "DataStructures.h"
#include "Quad.h"
#include "Sphere.h"

//Name is too long that is why using
using namespace rapidjson;

std::vector<LightData> g_lights;

std::vector<IGeometry*> g_geometry;

//Handle to memory of the geometry
//Put them close to help with cache
char* g_geometryMem;

std::vector<Instruction> g_animInstructions;

GlobalData g_data;
AnimMetaData g_animData;

glm::vec3 g_eye;


float g_aspectRatio;
float g_fieldOfView = 45.0f;

float g_colorRange = 255.f;
int g_framesToRender = 0;
float g_currentTime = 0.0f;

std::chrono::high_resolution_clock::time_point g_start;


bool LoadScene(std::string sceneName) {
	std::string sceneFile = ExternalFile::Load(sceneName);

	if (sceneFile.empty()) {
		std::cout << "Unable to find scene Description Try program again\n";
		//exit(EXIT_FAILURE);
		return false;
	}

	rapidjson::Document d;
	ParseResult result = d.Parse(sceneFile.c_str());
	if (!result) {
		fprintf(stderr, "JSON parse error: %s (%u)",
			GetParseError_En(result.Code()), result.Offset());
		//exit(EXIT_FAILURE);
		return false;
	}


	rapidjson::Value& SceneSettings = *GetValueByPointer(d, "/SceneSettings");
	g_data.Shininess = SceneSettings["SHININESS"].GetFloat();

	g_data.Antialias = SceneSettings["ANTIALIAS"].GetInt();


	//Try to speed this up
	g_data.bgColor.x = SceneSettings["BACKGROUND"][0].GetFloat();
	g_data.bgColor.y = SceneSettings["BACKGROUND"][1].GetFloat();
	g_data.bgColor.z = SceneSettings["BACKGROUND"][2].GetFloat();

	g_data.MaxDepth = SceneSettings["MAXDEPTH"].GetInt();

	g_data.Resolution.x = SceneSettings["RESOLUTION"][0].GetInt();
	g_data.Resolution.y = SceneSettings["RESOLUTION"][1].GetInt();


	//Get the Lights
	rapidjson::Value& lights = *GetValueByPointer(d, "/Lights");
	assert(lights.IsArray());

	for (int i = 0; i < lights.Size(); i++) {
		LightData light;

		light.Name = lights[i]["Name"].GetString();

		
		//This is pretty cool!
		light.Pos.x = lights[i]["Pos"][0].GetFloat();
		light.Pos.y = lights[i]["Pos"][1].GetFloat();
		light.Pos.z = lights[i]["Pos"][2].GetFloat();

		light.Diff.x = lights[i]["Diff"][0].GetFloat() / g_colorRange;
		light.Diff.y = lights[i]["Diff"][1].GetFloat() / g_colorRange;
		light.Diff.z = lights[i]["Diff"][2].GetFloat() / g_colorRange;

		light.Spec.x = lights[i]["Spec"][0].GetFloat() / g_colorRange;
		light.Spec.y = lights[i]["Spec"][1].GetFloat() / g_colorRange;
		light.Spec.z = lights[i]["Spec"][2].GetFloat() / g_colorRange;

		g_lights.push_back(light);
	}



	//Get Spheres
	rapidjson::Value& spheres = *GetValueByPointer(d, "/Spheres");
	assert(spheres.IsArray());

	rapidjson::Value& quads = *GetValueByPointer(d, "/Quads");
	assert(quads.IsArray());

	g_geometryMem = (char*)malloc((sizeof(Quad) * quads.Size()) + (sizeof(Sphere) * spheres.Size()));
	
	//Start of sphere memory
	char* sphereStartLocation = g_geometryMem + (sizeof(Quad) * quads.Size());

	for (int i = 0; i < spheres.Size(); i++) {
		int matType = spheres[i]["MatType"].GetInt();
		MatType type;

		if (matType == 0) {
			type = MatType::MIRROR;
		}
		else {
			type = MatType::DIFFUSE;
		}

		Sphere sphere = Sphere (
			glm::vec3(
				spheres[i]["Pos"][0].GetFloat(),
				spheres[i]["Pos"][1].GetFloat(),
				spheres[i]["Pos"][2].GetFloat()
			),
			spheres[i]["Radius"].GetFloat(),
		
			//Material
			glm::vec3(
				spheres[i]["Diff"][0].GetFloat() / g_colorRange,
				spheres[i]["Diff"][1].GetFloat() / g_colorRange,
				spheres[i]["Diff"][2].GetFloat() / g_colorRange
			),
			glm::vec3(
				spheres[i]["Spec"][0].GetFloat() / g_colorRange,
				spheres[i]["Spec"][1].GetFloat() / g_colorRange,
				spheres[i]["Spec"][2].GetFloat() / g_colorRange
			),
			glm::vec3(
				spheres[i]["Amb"][0].GetFloat() / g_colorRange,
				spheres[i]["Amb"][1].GetFloat() / g_colorRange,
				spheres[i]["Amb"][2].GetFloat() / g_colorRange
			),
			type,
			spheres[i]["Name"].GetString()
		);


		char* newSphere = sphereStartLocation + (i * sizeof(Sphere));

		memcpy(newSphere, &sphere, sizeof(Sphere));

		g_geometry.push_back((IGeometry*)newSphere);

	}





	for (int i = 0; i < quads.Size(); i++) {
		int matType = quads[i]["MatType"].GetInt();
		MatType type;

		if (matType == 0) {
			type = MatType::MIRROR;
		}
		else {
			type = MatType::DIFFUSE;
		}

		//yes this is a constructor
		//I am sorry for its size, and lack of error checking
		Quad quad = Quad(

			//Positions	
			glm::vec3(
			quads[i]["Pos1"][0].GetFloat(),
				quads[i]["Pos1"][1].GetFloat(),
				quads[i]["Pos1"][2].GetFloat()
			),

			glm::vec3(
				quads[i]["Pos2"][0].GetFloat(),
				quads[i]["Pos2"][1].GetFloat(),
				quads[i]["Pos2"][2].GetFloat()
			),

			glm::vec3(
				quads[i]["Pos3"][0].GetFloat(),
				quads[i]["Pos3"][1].GetFloat(),
				quads[i]["Pos3"][2].GetFloat()
			),

			//diffuse
			glm::vec3(
				quads[i]["Diff"][0].GetFloat() / g_colorRange,
				quads[i]["Diff"][1].GetFloat() / g_colorRange,
				quads[i]["Diff"][2].GetFloat() / g_colorRange
			),

			//Specular
			glm::vec3(
				quads[i]["Spec"][0].GetFloat() / g_colorRange,
				quads[i]["Spec"][1].GetFloat() / g_colorRange,
				quads[i]["Spec"][2].GetFloat() / g_colorRange
			),
			//Ambient
			glm::vec3 (
				quads[i]["Amb"][0].GetFloat() / g_colorRange,
				quads[i]["Amb"][1].GetFloat() / g_colorRange,
				quads[i]["Amb"][2].GetFloat() / g_colorRange
			),
				//material type
			type,
			quads[i]["Name"].GetString()
		);
		char* newQuad = (char*)g_geometryMem + (i * sizeof(Quad));

		memcpy(newQuad, &quad, sizeof(Quad));

		g_geometry.push_back((IGeometry*)newQuad);
	}
	return true;
}

void LoadAnimationDesc(std::string file) {
	//Set variables that are necessary

	std::string sceneFile = ExternalFile::Load(file);
	if (sceneFile.empty()) {
		std::cout << "no animation specified or could not be found\n";
		return;
	}
	g_framesToRender = 1;
	rapidjson::Document d;
	ParseResult result = d.Parse(sceneFile.c_str());
	if (!result) {
		fprintf(stderr, "JSON parse error: %s (%u)",
			GetParseError_En(result.Code()), result.Offset());
		exit(EXIT_FAILURE);
		return;
	}


	rapidjson::Value& MetaData = *GetValueByPointer(d, "/MetaData");
	g_animData.Fps = MetaData["Fps"].GetInt();
	g_animData.Time = MetaData["Time"].GetFloat();

	rapidjson::Value& Instructions = *GetValueByPointer(d, "/Instructions");
	assert(Instructions.IsArray());

	for (int i = 0; i < Instructions.Size(); i++) {
		Instruction instruction;
		
		
		instruction.Name = Instructions[i]["Name"].GetString();
		instruction.StartT = Instructions[i]["StartT"].GetFloat();
		instruction.EndT = Instructions[i]["EndT"].GetFloat();
		instruction.MoveType = Instructions[i]["MoveType"].GetFloat();

		instruction.StartPos.x = Instructions[i]["StartPos"][0].GetFloat();
		instruction.StartPos.y = Instructions[i]["StartPos"][1].GetFloat();
		instruction.StartPos.z = Instructions[i]["StartPos"][2].GetFloat();

	
		instruction.EndPos.x = Instructions[i]["EndPos"][0].GetFloat();
		instruction.EndPos.y = Instructions[i]["EndPos"][1].GetFloat();
		instruction.EndPos.z = Instructions[i]["EndPos"][2].GetFloat();

		instruction.LocalTime = 0.0f;

		g_animInstructions.push_back(instruction);
	}

	g_framesToRender = g_animData.Fps * g_animData.Time;
}

Ray CalculateRay(int i, int j) {

	//NOTE, I and J are flipped as I would like to process each row instead of column first
	glm::vec3 point = glm::vec3(0, 0, 0);

	float fov = glm::tan(glm::radians(g_fieldOfView) / 2);

	float imageX = j - (g_data.Resolution.x / 2.0f);
	float imageY = i - (g_data.Resolution.y / 2.0f);

	point.x = g_aspectRatio  * ((2 * imageX) / g_data.Resolution.x) * fov;
	point.y =  ((2 * imageY)/ g_data.Resolution.y) * -fov;

	//Set viewing plane 1 unit ahead of the camera
	point.z = g_eye.z + 1;

	Ray ray;
	ray.dir = glm::normalize(point - g_eye);

	ray.origin = g_eye;
	return ray;
}


//Returns a properly filled in hit if there is an intersection
//Otherwise it returns false and hit is undefined
bool FirstIntersection(Ray& ray, Hit& hit) {

	float closestT = std::numeric_limits<float>::max();

	Hit finalHit;

	//Check all geometry
	for (int i = 0; i < g_geometry.size(); i++) {
		Hit tempHit;
		if (g_geometry[i]->CalculateIntersection(ray, tempHit)) {
			if (tempHit.t < closestT && tempHit.t >= 0.01) {
				tempHit.mat = &g_geometry[i]->m_Material;
				closestT = tempHit.t;
				memcpy(&finalHit, &tempHit, sizeof(Hit));
			}
		}
	}

	memcpy(&hit, &finalHit, sizeof(Hit));
	//Check if the material has been set, if so we have a hit
	return !(hit.mat == nullptr);
}

glm::vec3 CalculateSingleLight(Hit hit, LightData light, Ray ray) {
	
	glm::vec3 lightVec = glm::normalize(light.Pos - hit.location);
	
	//Diffuse
	glm::vec3 result = light.Diff * hit.mat->Diff * glm::clamp(glm::dot(lightVec, hit.normal), 0.0f, 1.0f);

	glm::vec3 view = glm::normalize(hit.location - ray.origin);

	//Calculate the reflected vector
	glm::vec3 reflected = glm::normalize(glm::reflect(lightVec, hit.normal));

	//Specular
	result += light.Spec * hit.mat->Spec * pow(glm::clamp(dot(view, reflected ), 0.0f, 1.0f), g_data.Shininess);

	return result;
}

glm::vec3 CalculateLighting(Ray ray, Hit hit) {
	glm::vec3 color = glm::vec3(0);

	for (int i = 0; i < g_lights.size(); i++) {

		Ray shadowRay;
		shadowRay.origin = hit.location;

		glm::vec3 lightVec = g_lights[i].Pos - hit.location;
		shadowRay.dir = glm::normalize(g_lights[i].Pos - hit.location);

		Hit shadowHit = Hit();
		
		if (FirstIntersection(shadowRay, shadowHit)) {
			//Check distance between light and hit if light is behind hit then do not add color
			//so there is a shadow

			//Use sqrmagnitude in these calculations for speed
			float lightVecMag = glm::dot(lightVec, lightVec);

			glm::vec3 hitVec = shadowHit.location - hit.location;
			float hitvecMag = glm::dot(hitVec, hitVec);

			if (hitvecMag >= lightVecMag) {
				color += CalculateSingleLight(hit, g_lights[i], ray);
			}
		}
		else {
			//If no intersection just draw
			color += CalculateSingleLight(hit, g_lights[i], ray);
		}
	}

	return color;
}

glm::vec3 TraceRay(Ray ray, int maxDepth) {

	Hit hit = Hit();
	bool p = FirstIntersection(ray, hit); //get the first one
	if (!p) return glm::vec3(0);//out of scene


	//Calculate the local lighting
	glm::vec3 color = CalculateLighting(ray, hit);


	//Calculate more balances if necessary
	if (maxDepth > 0 && hit.mat->MatType == MatType::MIRROR) {
		Ray newRay;
		newRay.origin = hit.location;
		newRay.dir = glm::reflect(ray.dir, hit.normal);

		//Reduce brightness on each bounce
		color += 0.75f * TraceRay(newRay, maxDepth - 1);
	}

	return glm::clamp(color, 0.0f, 1.0f);
}


//Moves the spheres based on instruction in between the frame
void SetUpNextFrame() {
	g_currentTime += 1.0f/(float)g_animData.Fps;

	//Loop through all instructions to determine where to move each geometry
	for (int i = 0; i < g_animInstructions.size(); i++) {
		//Find which object we are trying to move
		for (int j = 0; j < g_geometry.size(); j++) {
			if (g_geometry[j]->m_Name.compare(g_animInstructions[i].Name) == 0) {
				//Check if this animation is active
				Instruction* instruction = &g_animInstructions[i];
				if ((g_currentTime >=  instruction->StartT) && (g_currentTime <= instruction->EndT + 0.0001f)) {
					Sphere* sphere = dynamic_cast<Sphere*>(g_geometry[j]);
					if (sphere) {
						instruction->LocalTime += 1.0f / (float)g_animData.Fps;
						if (instruction->MoveType == 0) {
							sphere->m_pos = glm::lerp(instruction->StartPos, instruction->EndPos, instruction->LocalTime / (instruction->EndT - instruction->StartT));
						}
						//If move type is one it is moving based on a sin wave
						//Intensity/affected axis is specified by EndPos (0,150,0) means move in a sin wave on the yaxis with a delta of -150 and 150
						//This is the way because it is alot of work on loading files, and my main focuse is the ray tracer/ animation
						else if (instruction->MoveType == 1) {
							sphere->m_pos = (instruction->EndPos * glm::sin(instruction->LocalTime * 3.14159f)) + instruction->StartPos;
						}
					}
				}
			}	
		}
	}
}






int main(int* argc, char** argv) {
	std::string sceneName = "";
	std::string animationName = "";

	std::cout << "Enter the scene File .Json file: ";
	std::cin >> sceneName;
	std::cout << "Enter the Animation .Json file(Type NA for no file): ";
	std::cin >> animationName;


	int frameCount = 0;

	g_start = std::chrono::high_resolution_clock::now();
	g_eye = glm::vec3(0, 0, -500);


	if (!LoadScene(sceneName)) {
		int stop = 0;

		std::cout << "type anything and Press enter to Continue...\n";
		std::cin >> stop;
		return 0;
	}

	LoadAnimationDesc(animationName);

	std::cout << "Rendering Scene\n";

	g_aspectRatio = g_data.Resolution.x / g_data.Resolution.y;

	float totalTimeRendering = 0.0f;
	int framesRendererd = 0;

	int filePaddingZeroCount = std::to_string(g_framesToRender).length();

	do {
		ExternalFile newFile = ExternalFile();


		newFile.BMPInit(g_data.Resolution.x, g_data.Resolution.y);

		#pragma omp parallel for
		for (int i = 0; i < (int)g_data.Resolution.y; i++) {
			for (int j = 0; j < (int)g_data.Resolution.x; j++) {

				Ray ray = CalculateRay(i, j); //get the primary ray
				glm::vec3 color = TraceRay(ray, g_data.MaxDepth);
				CV::ColorBMP realColor = CV::ColorBMP(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f);
				newFile.BMPBuffer(i, j, realColor);
			}
		}

		//Pad string with zeros.  We assume that 
		std::string frameNumberString = std::to_string(frameCount);
		std::string finalNumber = std::string(filePaddingZeroCount - frameNumberString.length(), '0') + frameNumberString;

		std::string fileName = "OutPut" + finalNumber + ".bmp";
		newFile.BMPWrite(fileName);
		frameCount++;

		double end = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - g_start).count();

		totalTimeRendering += end / 10000000.0;
		std::cout << "Frame Completed in: " << end / 10000000.0 << " Seconds" << std::endl;
		g_start = std::chrono::high_resolution_clock::now();



		g_framesToRender--;

		//How to avoid double Check
		if (g_framesToRender >= 0) {
			SetUpNextFrame();
		}
		framesRendererd++;
	} while (g_framesToRender >= 0);
	std::cout << "Frame Avg: " << totalTimeRendering / (float)framesRendererd << " Seconds" << std::endl;

	int stop = 0;

	std::cout << "type anything and Press enter to Continue...\n";
	std::cin >> stop;
	return 0;
}