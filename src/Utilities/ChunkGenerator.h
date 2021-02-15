#pragma once
#include <Scene.h>
#include <Application.h>
#include <ObjLoader.h>
#include <RendererComponent.h>
#include <Transform.h>
#include <vector>
#include <math.h>

#include <IBehaviour.h>
#include "Utilities/Util.h"

#include <Behaviours/RotateObjectBehaviour.h>
#include "MeshBuilder.h"

class ChunkGenerator abstract
{
public:
	
	static float DegreesToRadians(float _degrees);
	static float RadiansToDegrees(float _radians);

	//Regenerates environment with your settings
	static void RegenerateEnvironment();
	//Generates an environment with your settings
	static void GenerateEnvironment();
	//Cleans up the environment using your settings
	static void CleanEnvironment();
	
	static void CleanUpPointers();

	//Adds object to generation
	static void AddObjectToGeneration(std::string fileName, std::vector<ShaderMaterial::sptr> objMat, int numToSpawn, int _coordinateX, int _coordinateY);
	static void AddProps(std::string fileName, ShaderMaterial::sptr objMat);
	static void SpawnProp(glm::vec3 position, int spawnCount);
	//Removes object from generation
	static void RemoveObjectFromGeneration(std::string fileName);

	static std::vector<std::string> GetObjectsOnList();

private:
	//The gameobjects spawned here
	static std::vector<std::vector<GameObject>> _objectsSpawned;
	static std::vector<std::vector<GameObject>> _propsSpawned;

	//The vaos to spawn in
	static std::vector<VertexArrayObject::sptr> _vaosToSpawn;
	static std::vector<bool> _loadedIn;
	static std::vector<ShaderMaterial::sptr> _materialsForSpawning;
	static std::vector<ShaderMaterial::sptr> _propMaterialsForSpawning;
	static std::vector<int> _numToSpawn;

	static std::vector<VertexArrayObject::sptr> _propVAOS;

	//Allows us to go through and remove from list
	static std::vector<std::string> _objectsToSpawn;
	static std::vector<std::string> _propsToSpawn;

	static std::vector<int> coordinateX;
	static std::vector<int> coordinateY;

	static float frequency1;
	static float height1;
	static float frequency2;
	static float height2;
	static float frequency3;
	static float height3;

	////////Not Implemented/////
	//static std::vector<char> _letterRepresentation;
	//static std::vector<std::vector<char>> _generatedMapPlacements;
	//static std::vector<std::vector<float>> _generatedMapHeight;
};