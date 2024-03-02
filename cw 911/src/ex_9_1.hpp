#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <random>


#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "SOIL/SOIL.h"

#define GLT_IMPLEMENTATION
#include "../gltext.h"

const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

int WIDTH = 500, HEIGHT = 500;


namespace textures {
	// Planets
	GLuint mercury;
	GLuint venus;
	GLuint earth;
	GLuint mars;
	GLuint jupiter;
	GLuint saturn;
	GLuint uranus;
	GLuint neptune;

	// Others
	GLuint skybox;
	GLuint moon;
	GLuint sun;
	GLuint ship;
	GLuint coin;
	GLuint green;

	// Normals
	GLuint planetNormal;
	GLuint earthNormal;
	GLuint moonNormal;
	GLuint sunNormal;
	GLuint shipNormal;
	GLuint ufoNormal;
}

namespace models {
	Core::RenderContext spaceshipContext;
	Core::RenderContext sphereContext;
	Core::RenderContext testContext;
	Core::RenderContext ufoContext;
	Core::RenderContext ringContext;
	Core::RenderContext coinContext;
}

GLuint depthMapFBO;
GLuint depthMap;

GLuint program;
GLuint programSun;
GLuint programTest;
GLuint programTex;
GLuint programSkybox;

Core::Shader_Loader shaderLoader;

Core::RenderContext shipContext;
Core::RenderContext sphereContext;
Core::RenderContext cubeContext;
Core::RenderContext ufoContext;
Core::RenderContext ringContext;
Core::RenderContext coinContext;

glm::vec3 sunPos = glm::vec3(0.f, 0.f, 0.f);
glm::vec3 sunDir = glm::vec3(0.5f, 1.f, 0.5f);
glm::vec3 sunColor = glm::vec3(0.9f, 0.9f, 0.7f) * 3;

glm::vec3 cameraPos = glm::vec3(0.479490f, 1.250000f, -2.124680f);
glm::vec3 cameraDir = glm::vec3(-0.354510f, 0.000000f, 0.935054f);


glm::vec3 spaceshipPos = glm::vec3(-27.f, 2.f, 14.f);
glm::vec3 spaceshipDir = glm::vec3(0.997510f, 0.000000f, -0.070568f);

GLuint quadVAO, quadVBO;

float tiltAngleSide;

float aspectRatio = 1.f;

float exposition = 1.f;

float Qkeytrick = 0.00f;

float Ekeytrick = 0.00f;

glm::vec3 pointlightPos = glm::vec3(0, 0, 0);
glm::vec3 pointlightColor = glm::vec3(0.94, 0.92, 0.78) * 10000;

glm::vec3 spotlightPos = glm::vec3(0, 0, 0);
glm::vec3 spotlightConeDir = glm::vec3(0, 0, 0);
glm::vec3 spotlightColor = glm::vec3(0.4, 0.4, 0.9) * 10;


float spotlightPhi = 3.14 / 4;


float lastTime = -1.f;
float deltaTime = 0.f;
float orbitRadius = 5.0f;
float orbitSpeed = 0.01f;

float addToMoveSpeed = 0.00f;


int healthpoints = 3;
float money = 0;
float shipaccelerationdebt = 0;




bool game = true;

std::vector<std::pair<glm::vec3, float>> objectsPos;
std::vector<std::tuple<glm::mat4, float, float>> coinsPos;



void updateDeltaTime(float time) {
	if (lastTime < 0) {
		lastTime = time;
		return;
	}

	deltaTime = time - lastTime;
	if (deltaTime > 0.1) deltaTime = 0.1;
	lastTime = time;
}

GLuint loadSkybox(const std::vector<std::string>& filepaths) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int w, h;

	for (unsigned int i = 0; i < 6; i++) {
		unsigned char* image = SOIL_load_image(filepaths[i].c_str(), &w, &h, 0, SOIL_LOAD_RGBA);

		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image
		);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

glm::mat4 createCameraMatrix()
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide, cameraDir));
	glm::mat4 cameraRotrationMatrix = glm::mat4({
		cameraSide.x,cameraSide.y,cameraSide.z,0,
		cameraUp.x,cameraUp.y,cameraUp.z ,0,
		-cameraDir.x,-cameraDir.y,-cameraDir.z,0,
		0.,0.,0.,1.,
		});
	cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
	glm::mat4 cameraMatrix = cameraRotrationMatrix * glm::translate(-cameraPos);

	return cameraMatrix;
}

glm::mat4 createPerspectiveMatrix()
{
	glm::mat4 perspectiveMatrix;
	float n = 0.05;
	float f = 200.;
	float fov = 90.;
	float S = 1 / glm::tan(fov / 2 * glm::pi<float>() / 180);
	float a1 = glm::min(aspectRatio, 1.f);
	float a2 = glm::min(1 / aspectRatio, 1.f);
	perspectiveMatrix = glm::mat4({
		S,0.,0.,0.,
		0.,aspectRatio * S,0.,0.,
		0.,0.,(f + n) / (n - f),2 * f * n / (n - f),
		0.,0.,-1.,0.,
		});


	perspectiveMatrix = glm::transpose(perspectiveMatrix);

	return perspectiveMatrix;
}

bool checkCollision(const glm::vec3& pos1, float radius1, const glm::vec3& pos2, float radius2) {
	float distance = glm::distance(pos1, pos2);
	float sumOfRad = radius1 + radius2;
	return distance < sumOfRad;
}

void drawObjectPBR(Core::RenderContext& context, glm::mat4 modelMatrix, /*glm::vec3 color*/GLuint textureID, GLuint normalmapID, float roughness, float metallic) {

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	glUniform1f(glGetUniformLocation(program, "exposition"), exposition);

	glUniform1f(glGetUniformLocation(program, "roughness"), roughness);
	glUniform1f(glGetUniformLocation(program, "metallic"), metallic);

	//glUniform3f(glGetUniformLocation(program, "color"), color.x, color.y, color.z);

	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glUniform3f(glGetUniformLocation(program, "sunDir"), sunDir.x, sunDir.y, sunDir.z);
	glUniform3f(glGetUniformLocation(program, "sunColor"), sunColor.x, sunColor.y, sunColor.z);

	glUniform3f(glGetUniformLocation(program, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	glUniform3f(glGetUniformLocation(program, "lightColor"), pointlightColor.x, pointlightColor.y, pointlightColor.z);

	glUniform3f(glGetUniformLocation(program, "spotlightConeDir"), spotlightConeDir.x, spotlightConeDir.y, spotlightConeDir.z);
	glUniform3f(glGetUniformLocation(program, "spotlightPos"), spotlightPos.x, spotlightPos.y, spotlightPos.z);
	glUniform3f(glGetUniformLocation(program, "spotlightColor"), spotlightColor.x, spotlightColor.y, spotlightColor.z);
	glUniform1f(glGetUniformLocation(program, "spotlightPhi"), spotlightPhi);
	Core::SetActiveTexture(textureID, "colorTexture", program, 0);
	Core::SetActiveTexture(normalmapID, "normalTexture", program, 1);
	Core::DrawContext(context);

}

void drawObjectSkybox(Core::RenderContext& context, glm::mat4 modelMatrix) {
	glUseProgram(programSkybox);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programSkybox, "transformation"), 1, GL_FALSE, (float*)&transformation);
	Core::DrawContext(context);
}






void renderShadowapSun() {
	float time = glfwGetTime();
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
}




void renderScene(GLFWwindow* window)
{
	glClearColor(0.4f, 0.4f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	float time = glfwGetTime();
	updateDeltaTime(time);
	renderShadowapSun();


	glBindTexture(GL_TEXTURE_CUBE_MAP, textures::skybox);
	glm::mat4 skyboxModelMatrix = glm::translate(cameraPos);
	drawObjectSkybox(cubeContext, skyboxModelMatrix);
	glClear(GL_DEPTH_BUFFER_BIT);

	
	glUseProgram(programSun);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * glm::translate(pointlightPos) * glm::scale(glm::vec3(15.0));
	glUniformMatrix4fv(glGetUniformLocation(programSun, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniform3f(glGetUniformLocation(programSun, "color"), sunColor.x / 2, sunColor.y / 2, sunColor.z / 2);
	glUniform1f(glGetUniformLocation(programSun, "exposition"), exposition);

	glUniform1i(glGetUniformLocation(programSun, "sunTexture"), 0);
	Core::SetActiveTexture(textures::sun, "sunTexture", programSun, 0);

	Core::DrawContext(sphereContext);

	glUseProgram(0);

	glUseProgram(program);

	objectsPos.clear();

	
	



	// PLANETS, MOONS AND ASTEROIDS
	if (game == true) {
		objectsPos.emplace_back(glm::vec3(0., 0., 0.), 16); //SUN

		//first custom planetoid that we have to dodge from

	

		
		glm::mat4 firstPlanetoidMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time/3) * glm::translate(glm::vec3(190.f, 0, 0)) * glm::scale(glm::vec3(3.15f));

		drawObjectPBR(sphereContext,
			firstPlanetoidMatrix,
			textures::mars, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 firstPlanetoidPos(firstPlanetoidMatrix[3]);
		objectsPos.emplace_back(firstPlanetoidPos, 0.36);
		

		//second custom planetoid that we have to dodge from
		float speed2 = time / 2.5;
		
		glm::mat4 secondPlanetoidMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(speed2) * glm::translate(glm::vec3(220.f, 0, 0)) * glm::scale(glm::vec3(5.2f));

		drawObjectPBR(sphereContext,
			secondPlanetoidMatrix,
			textures::neptune, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 secondPlanetoidPos(secondPlanetoidMatrix[3]);
		objectsPos.emplace_back(secondPlanetoidPos, 0.60);

		//mercury that we have to dodge from

		glm::mat4 mercuryMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time/2) * glm::translate(glm::vec3(250.f, 0, 0)) * glm::scale(glm::vec3(15.7f));

		drawObjectPBR(sphereContext,
			mercuryMatrix,
			textures::mercury, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 mercuryPos(mercuryMatrix[3]);
		objectsPos.emplace_back(mercuryPos, 1.78);

		glm::mat4 venusMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 3) * glm::translate(glm::vec3(350.f, 0, 0)) * glm::scale(glm::vec3(15.7f));

		drawObjectPBR(sphereContext,
			venusMatrix,
			textures::venus, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 venusPos(venusMatrix[3]);
		objectsPos.emplace_back(venusPos, 1.78);

		glm::mat4 earthMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 4) * glm::translate(glm::vec3(550.f, 0, 0)) * glm::scale(glm::vec3(45.7f));

		drawObjectPBR(sphereContext,
			earthMatrix,
			textures::earth, textures::earthNormal,
			0.3, 0.5);

		glm::vec3 earthPos(earthMatrix[3]);
		objectsPos.emplace_back(earthPos, 4.65);

		glm::mat4 moonMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 5) * glm::translate(glm::vec3(550.f, 0, 0)) * glm::eulerAngleY(time / 25000) * glm::translate(glm::vec3(80.0f, 0, 0)) * glm::rotate(time, glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(6.7f));

		drawObjectPBR(sphereContext,
			moonMatrix,
			textures::moon, textures::moonNormal,
			0.7, 0.5);

		glm::vec3 moonPos(moonMatrix[3]);
		objectsPos.emplace_back(moonPos, 0.85);

		glm::mat4 marsMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 6) * glm::translate(glm::vec3(800.f, 0, 0)) * glm::scale(glm::vec3(25.7f));

		drawObjectPBR(sphereContext,
			marsMatrix,
			textures::mars, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 marsPos(marsMatrix[3]);
		objectsPos.emplace_back(marsPos, 2.72);

		glm::mat4 moonMarsMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 6) * glm::translate(glm::vec3(800.f, 0, 0)) * glm::eulerAngleY(time) * glm::translate(glm::vec3(80.0f, 0, 0)) * glm::rotate(time, glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(6.7f));

		drawObjectPBR(sphereContext,
			moonMarsMatrix,
			textures::moon, textures::moonNormal,
			0.7, 0.5);

		glm::vec3 moonMarsPos(moonMarsMatrix[3]);
		objectsPos.emplace_back(moonMarsPos, 0.85);

		glm::mat4 jupiterMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 5) * glm::translate(glm::vec3(1500.f, 0, 0)) * glm::scale(glm::vec3(120.7f));

		drawObjectPBR(sphereContext,
			jupiterMatrix,
			textures::jupiter, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 jupiterPos(jupiterMatrix[3]);
		objectsPos.emplace_back(jupiterPos, 12);

		glm::mat4 saturnMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 5) * glm::translate(glm::vec3(2300.f, 0, 0)) * glm::scale(glm::vec3(130.7f));

		drawObjectPBR(sphereContext,
			saturnMatrix,
			textures::saturn, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 saturnPos(saturnMatrix[3]);
		objectsPos.emplace_back(saturnPos, 12.8);

		drawObjectPBR(ringContext,
			glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 5) * glm::translate(glm::vec3(2300.f, 0, 0)) * glm::scale(glm::vec3(190.7f)),
			textures::saturn, textures::planetNormal,
			0.3, 0.5);

		glm::mat4 uranusMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 6) * glm::translate(glm::vec3(2700.f, 0, 0)) * glm::scale(glm::vec3(100.7f));

		drawObjectPBR(sphereContext,
			uranusMatrix,
			textures::uranus, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 uranusPos(uranusMatrix[3]);
		objectsPos.emplace_back(uranusPos, 10.12);

		glm::mat4 neptuneMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(time / 6) * glm::translate(glm::vec3(3600.f, 0, 0)) * glm::scale(glm::vec3(100.7f));

		drawObjectPBR(sphereContext,
			neptuneMatrix,
			textures::neptune, textures::planetNormal,
			0.3, 0.5);

		glm::vec3 neptunePos(neptuneMatrix[3]);
		objectsPos.emplace_back(neptunePos, 10.14);

		glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
		glm::vec3 spaceshipUp = glm::normalize(glm::cross(spaceshipSide, spaceshipDir));
		glm::mat4 specshipCameraRotrationMatrix = glm::mat4({
			spaceshipSide.x,spaceshipSide.y,spaceshipSide.z,0,
			spaceshipUp.x,spaceshipUp.y,spaceshipUp.z ,0,
			-spaceshipDir.x,-spaceshipDir.y,-spaceshipDir.z,0,
			0.,0.,0.,1.,
			});

		// SHIP
		
		glm::mat4 shipMatrix = glm::translate(spaceshipPos) * specshipCameraRotrationMatrix * glm::eulerAngleY(glm::pi<float>()) * glm::rotate(glm::mat4(), tiltAngleSide * glm::radians(30.0f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.03f));

		drawObjectPBR(shipContext,
			shipMatrix,
			textures::ship, textures::ufoNormal,
			0.3, 0.5
		);

		//COINS
		printf("coinsPos.size = %d\n", coinsPos.size());
		for (size_t i = 0; i < coinsPos.size(); i++) {
			glm::mat4 matrix;
			float whenlastscored;
			float coinvalue;
			std::tie(matrix, whenlastscored, coinvalue) = coinsPos[i];
			glm::vec3 coinPos(matrix[3]);

			float r = 0.5;
			float m = 0.2;

			if (time - whenlastscored > 5) {

				if (checkCollision(coinPos, 0.5, spaceshipPos, 0.08)) {
					//whenlastscored = time;
					std::get<1>(coinsPos[i]) = time;
					money+=coinvalue;
					shipaccelerationdebt += coinvalue;
					r=1.0f;
					m=1.0f;
				}
				
				if (coinvalue == 1) {

					drawObjectPBR(coinContext,
						matrix,
					textures::ship, textures::planetNormal,
						r, m);
				}
				else if (coinvalue == 5) {

					drawObjectPBR(coinContext,
						matrix,
					textures::coin, textures::planetNormal,
						r, m);

				}
				else if (coinvalue == 50) {

					drawObjectPBR(coinContext,
						matrix,
					textures::green, textures::planetNormal,
						r, m);

				}
			}
		}

	

		//COLLISIONS

		for (size_t i = 0; i < objectsPos.size(); i++) {
			if (checkCollision(objectsPos[i].first, objectsPos[i].second, spaceshipPos, 0.08)) {
				healthpoints--;
				spaceshipPos = glm::vec3(-27.f, 0.f, 14.f); //respawn point
			}
		}	

		if (healthpoints == 0) {
			game=false; //GAME OVER
		}

		spotlightPos = spaceshipPos + 0.2 * spaceshipDir;
		spotlightConeDir = spaceshipDir;

		// Initialize glText
		gltInit();

		// Creating text
		GLTtext* text = gltCreateText();
		std::string textvalue = "MONEY: " + std::to_string(money);

		gltSetText(text, textvalue.c_str());

		GLTtext* text1 = gltCreateText();
		std::string text1value = "LIFES LEFT: " + std::to_string(healthpoints);

		gltSetText(text1, text1value.c_str());


		// Begin text drawing (this for instance calls glUseProgram)
		gltBeginDraw();

		// Draw any amount of text between begin and end
		gltColor(1.0f, 1.0f, 1.0f, 1.0f);
		gltDrawText2D(text, 0.5f, 0.2f, 1.0f);
		gltDrawText2D(text1, 0.5f, 20.2f, 1.0f);


		// Finish drawing text
		gltEndDraw();

		// Deleting text
		gltDeleteText(text);

		// Destroy glText
		gltTerminate();
	}
	else {
		glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
		glm::vec3 spaceshipUp = glm::normalize(glm::cross(spaceshipSide, spaceshipDir));
		glm::mat4 specshipCameraRotrationMatrix = glm::mat4({
			spaceshipSide.x,spaceshipSide.y,spaceshipSide.z,0,
			spaceshipUp.x,spaceshipUp.y,spaceshipUp.z ,0,
			-spaceshipDir.x,-spaceshipDir.y,-spaceshipDir.z,0,
			0.,0.,0.,1.,
			});

		glm::mat4 shipMatrix = glm::translate(spaceshipPos) * specshipCameraRotrationMatrix * glm::eulerAngleY(glm::pi<float>()) * glm::rotate(glm::mat4(), tiltAngleSide * glm::radians(30.0f), glm::vec3(0, 0, 1)) * glm::scale(glm::vec3(0.03f));

		drawObjectPBR(shipContext,
			shipMatrix,
			textures::ship, textures::ufoNormal,
			0.3, 0.5
		);

		// Initialize glText
		gltInit();

		// Creating text
		GLTtext* text = gltCreateText();
		std::string textvalue = "GAME OVER";

		gltSetText(text, textvalue.c_str());

		GLTtext* text1 = gltCreateText();
		std::string text1value = "score: " + std::to_string(money);

		gltSetText(text1, text1value.c_str());


		// Begin text drawing (this for instance calls glUseProgram)
		gltBeginDraw();

		// Draw any amount of text between begin and end
		gltColor(1.0f, 0.0f, 0.0f, 1.0f);
		gltDrawText2D(text, 220.5f, 30.2f, 1.0f);
		gltDrawText2D(text1, 190.5f, 50.2f, 1.0f);


		// Finish drawing text
		gltEndDraw();

		// Deleting text
		gltDeleteText(text);

		// Destroy glText
		gltTerminate();


	}

	glUseProgram(0);
	glfwSwapBuffers(window);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
	WIDTH = width;
	HEIGHT = height;
}
void loadModelToContext(std::string path, Core::RenderContext& context)
{
	Assimp::Importer import;
	const aiScene * scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	context.initFromAssimpMesh(scene->mMeshes[0]);
}

void init(GLFWwindow* window)
{
	srand(time(NULL)); //For coins

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glEnable(GL_DEPTH_TEST);

	program = shaderLoader.CreateProgram("shaders/shader_9_1.vert", "shaders/shader_9_1.frag");
	programTest = shaderLoader.CreateProgram("shaders/test.vert", "shaders/test.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_8_sun.vert", "shaders/shader_8_sun.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");

	loadModelToContext("./models/sphere.obj", sphereContext);
	loadModelToContext("./models/spaceship.obj", shipContext);
	loadModelToContext("./models/cube.obj", cubeContext);
	loadModelToContext("./models/ufo.obj", ufoContext);
	loadModelToContext("./models/ring.obj", ringContext);
	loadModelToContext("./models/star.obj", coinContext);

	std::vector<std::string> skyboxFaces = {
			
	"textures/skybox/left.png",

	"textures/skybox/right.png",
	"textures/skybox/top.png",
	"textures/skybox/bottom.png",
	"textures/skybox/back.png",
	"textures/skybox/front.png"
	};
	textures::skybox = loadSkybox(skyboxFaces);

	// Planets
	textures::mercury = Core::LoadTexture("textures/mercury.jpg");
	textures::venus = Core::LoadTexture("textures/venus.jpg");
	textures::earth = Core::LoadTexture("textures/earth.png");
	textures::mars = Core::LoadTexture("textures/mars.jpg");
	textures::jupiter = Core::LoadTexture("textures/jupiter.jpg");
	textures::saturn = Core::LoadTexture("textures/saturn.jpg");
	textures::uranus = Core::LoadTexture("textures/uranus.jpg");
	textures::neptune = Core::LoadTexture("textures/neptune.jpg");

	// Others
	textures::ship = Core::LoadTexture("textures/spaceship_metal.jpg");
	textures::moon = Core::LoadTexture("textures/moon.jpg");
	textures::sun = Core::LoadTexture("textures/sun.jpg");
	textures::coin = Core::LoadTexture("textures/coin_gold.jpg");
	textures::green = Core::LoadTexture("textures/green.jpg");
	

	// Normals
	textures::earthNormal = Core::LoadTexture("textures/earth_normalmap.png");
	textures::moonNormal = Core::LoadTexture("textures/moon_normal.jpg");
	textures::shipNormal = Core::LoadTexture("textures/spaceship_normal.jpg");
	textures::sunNormal = Core::LoadTexture("textures/sun_normal.jpg");
	textures::planetNormal = Core::LoadTexture("textures/planet_normal.png");
	textures::ufoNormal = Core::LoadTexture("textures/ufo_normal.jpg");

	glm::mat4 coinMatrix;
	// Coins positions

	for (int i = 0; i < 3; i++) {
		switch (i)
		{
		case 1: //mercury line
			for (float j = 1; j <= 360; j++) { //circa 2 * pi * r spots to put coins
				coinMatrix = glm::translate(pointlightPos) * glm::eulerAngleY(j) * glm::scale(glm::vec3(0.1)) * glm::translate(glm::vec3(190.f, 0, 0));
				if (rand()%100<40) {
					//glm::vec3 coinPos(coinMatrix[3]);
					coinsPos.emplace_back(coinMatrix, 0, 1);
				}
			}
			//break;
		case 2: // wenus line
			for (float j = 1; j <= 360; j++) //circa 2 * pi * r spots to put coins
			{
				coinMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(j) * glm::translate(glm::vec3(220.f, 0, 0));
				if (rand()%100<10) {
					//glm::vec3 coinPos(coinMatrix[3]);
					coinsPos.emplace_back(coinMatrix, 0, 5);
				}

			}
			//break;
		case 3: //earth line
			for (float j = 1; j <= 360; j++) //circa 2 * pi * r spots to put coins
			{
				coinMatrix = glm::translate(pointlightPos) * glm::scale(glm::vec3(0.1)) * glm::eulerAngleY(j) * glm::translate(glm::vec3(250.f, 0, 0));
				if (rand() % 100 <2) {
					//glm::vec3 coinPos(coinMatrix[3]);
					coinsPos.emplace_back(coinMatrix, 0, 50);
				}
			}
			//break;
		}

	}




}

void shutdown(GLFWwindow* window)
{
	shaderLoader.DeleteProgram(program);
}

void processInput(GLFWwindow* window)
{
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.015f;
	float moveSpeed = 0.05f + addToMoveSpeed;
		
	double x = 0.05;

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
		moveSpeed *= 4.0f;
	}
	
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		spaceshipPos += spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		spaceshipPos -= spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		if (Ekeytrick == 0) {
			Ekeytrick = 6.0;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		if (Qkeytrick == 0) {
			Qkeytrick = 6.0;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		spaceshipPos += spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		spaceshipPos -= spaceshipUp * moveSpeed;


	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		spaceshipDir = glm::vec3(glm::eulerAngleY(angleSpeed) * glm::vec4(spaceshipDir, 0));
		tiltAngleSide -= x * 0.8;
	}
	else {
		if (tiltAngleSide < 0 && Qkeytrick==0 && Ekeytrick == 0) {
			tiltAngleSide += x;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		spaceshipDir = glm::vec3(glm::eulerAngleY(-angleSpeed) * glm::vec4(spaceshipDir, 0));
		tiltAngleSide += x*0.8;
	}
	else {
		if (tiltAngleSide > 0 && Ekeytrick == 0 && Qkeytrick == 0) {
			tiltAngleSide -= x;
		}
	}
	
	if (Qkeytrick > 0) {

		spaceshipPos -= spaceshipSide * 0.05 * 2;
		tiltAngleSide -= x*8;
		printf("tiltAngleSide = %ff\n", tiltAngleSide);

		Qkeytrick -= x*4;
		if (Qkeytrick < 0) {
			Qkeytrick = 0;
		}
	}

	if (Ekeytrick > 0) {

		spaceshipPos += spaceshipSide * 0.05 * 2;
		tiltAngleSide += x*8;
		Ekeytrick -= x*4;
		if (Ekeytrick < 0) {
			Ekeytrick = 0;
		}
	}

	if (Ekeytrick == 0 && Qkeytrick == 0) {
		tiltAngleSide = fmaxf(-0.75, fminf(0.75, tiltAngleSide));
	}

	if (shipaccelerationdebt > 0) {
		addToMoveSpeed += 0.05 * 0.001;
		shipaccelerationdebt -= 1;
	}

	cameraPos = spaceshipPos - 0.5 * spaceshipDir + glm::vec3(0, 1, 0) * 0.1f;
	cameraDir = spaceshipDir;

	// PBR light
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		exposition -= 0.05;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		exposition += 0.05;

	// DEBUG
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
		printf("spaceshipPos = glm::vec3(%ff, %ff, %ff);\n", spaceshipPos.x, spaceshipPos.y, spaceshipPos.z);
		printf("spaceshipDir = glm::vec3(%ff, %ff, %ff);\n", spaceshipDir.x, spaceshipDir.y, spaceshipDir.z);
	}

	glfwSetCursorPos(window, WIDTH / 2, HEIGHT / 2);
}

void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		renderScene(window);
		glfwPollEvents();
	}
}
