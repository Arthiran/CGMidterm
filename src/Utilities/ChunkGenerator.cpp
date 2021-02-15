#include "ChunkGenerator.h"

#ifndef M_PI
#define M_PI (4.0 * std::atan2(1.0, 1.0))
#endif

//The gameobject references to the spawned objects
std::vector<std::vector<GameObject>> ChunkGenerator::_objectsSpawned;

//Object information for being spawned
std::vector<VertexArrayObject::sptr> ChunkGenerator::_vaosToSpawn;
std::vector<bool> ChunkGenerator::_loadedIn;
std::vector<ShaderMaterial::sptr> ChunkGenerator::_materialsForSpawning;
std::vector<int> ChunkGenerator::_numToSpawn;
std::vector<int> ChunkGenerator::coordinateX;
std::vector<int> ChunkGenerator::coordinateY;
std::vector<VertexArrayObject::sptr> ChunkGenerator::_propVAOS;
std::vector<std::string> ChunkGenerator::_propsToSpawn;
std::vector<ShaderMaterial::sptr> ChunkGenerator::_propMaterialsForSpawning;
std::vector<std::vector<GameObject>> ChunkGenerator::_propsSpawned;
float ChunkGenerator::frequency1;
float ChunkGenerator::height1;
float ChunkGenerator::frequency2;
float ChunkGenerator::height2;
float ChunkGenerator::frequency3;
float ChunkGenerator::height3;

//The filenames of the objects to spawn
std::vector<std::string> ChunkGenerator::_objectsToSpawn;

float ChunkGenerator::DegreesToRadians(float _degrees)
{
	return _degrees* M_PI / 180.0;
}

float ChunkGenerator::RadiansToDegrees(float _radians)
{
	return _radians * 180.0 / M_PI;
}

void ChunkGenerator::RegenerateEnvironment()
{
	CleanEnvironment();

	GenerateEnvironment();
}

void ChunkGenerator::GenerateEnvironment()
{
	frequency1 = Util::GetRandomNumberBetween(1.0f, 10.0f);
	height1 = Util::GetRandomNumberBetween(1.0f, 10.0f);
	frequency2 = Util::GetRandomNumberBetween(1.0f, 10.0f);
	height2 = Util::GetRandomNumberBetween(1.0f, 10.0f);
	frequency3 = Util::GetRandomNumberBetween(1.0f, 10.0f);
	height3 = Util::GetRandomNumberBetween(1.0f, 10.0f);
	for (int i = 0; i < _objectsToSpawn.size(); i++)
	{
		std::vector<GameObject> temp;
		{
			//Load in this object vao
			if (!_loadedIn[i])
			{
				VertexArrayObject::sptr vao = ObjLoader::LoadFromFile(_objectsToSpawn[i]);
				_vaosToSpawn.push_back(vao);
				_loadedIn[i] = true;
			}

			int spawnAmount = _numToSpawn[i];
			int spawnCounter = 0;

			for (int j = 0 + spawnAmount * coordinateX[i]; j <= spawnAmount + spawnAmount * coordinateX[i]; j++)
			{
				for (int k = 0 + spawnAmount * coordinateY[i]; k <= spawnAmount + spawnAmount * coordinateY[i]; k++)
				{
					float offset = 2.0f;

					float result = round(sin(DegreesToRadians(frequency1 * j)) * sin(DegreesToRadians(frequency1 * k)) * height1 + 
						sin(DegreesToRadians(frequency2 * j)) * sin(DegreesToRadians(frequency2 * k)) * height2 +
						sin(DegreesToRadians(frequency3 * j)) * sin(DegreesToRadians(frequency3 * k)) * height3) * offset;

					temp.push_back(Application::Instance().ActiveScene->CreateEntity(_objectsToSpawn[i] + (std::to_string(spawnCounter + 1))));
					if (result >= 10.0f)
					{
						temp[spawnCounter].emplace<RendererComponent>().SetMesh(_vaosToSpawn[i]).SetMaterial(_materialsForSpawning[2]);
					}
					else if (result <= -10.0f)
					{
						temp[spawnCounter].emplace<RendererComponent>().SetMesh(_vaosToSpawn[i]).SetMaterial(_materialsForSpawning[1]);
					}
					else
					{
						int randProp = Util::GetRandomNumberBetween(1.0f, 100.0f);
						if (randProp == 33)
						{
							SpawnProp(glm::vec3(j * offset, k * offset, result + 1), spawnCounter);
						}
						temp[spawnCounter].emplace<RendererComponent>().SetMesh(_vaosToSpawn[i]).SetMaterial(_materialsForSpawning[0]);
					}
					//int newX = j - spawnAmount * coordinateX;
					//int newY = k - spawnAmount * coordinateY;
					temp[spawnCounter].get<Transform>().SetLocalPosition(glm::vec3(j * offset, k * offset, result));
					spawnCounter++;
				}
			}
		}

		//Add object to the spawned list
		_objectsSpawned.push_back(temp);
	}
}

void ChunkGenerator::CleanEnvironment()
{
	//Remove all the entities
	for (int i = 0; i < _objectsSpawned.size(); i++)
	{
		for (int j = 0; j < _objectsSpawned[i].size(); j++)
		{
			Application::Instance().ActiveScene->RemoveEntity(_objectsSpawned[i][j]);
		}
	}

	//Remove all the entities
	for (int i = 0; i < _propsSpawned.size(); i++)
	{
		for (int j = 0; j < _propsSpawned[i].size(); j++)
		{
			Application::Instance().ActiveScene->RemoveEntity(_propsSpawned[i][j]);
		}
	}

	//Clear out objects spawned
	_propsSpawned.clear();
	_objectsSpawned.clear();
}

void ChunkGenerator::CleanUpPointers()
{
	//Clear up vao references so the smart pointers can clear
	_vaosToSpawn.clear();
	_propVAOS.clear();
	//Clear up material references so the smart pointers can clear
	_materialsForSpawning.clear();
	_propMaterialsForSpawning.clear();
}

void ChunkGenerator::AddObjectToGeneration(std::string fileName, std::vector<ShaderMaterial::sptr> objMat, int numToSpawn, int _coordinateX, int _coordinateY)
{
	//Find the filename in the list
	int index = Util::FindInVector(fileName, _objectsToSpawn);
	//If the filename was found in the list we ain't adding it again
	/*if (index != -1)
	{
		printf("Object already found in list\n");
		return;
	}*/

	//Loads in the mesh and adds to list
	VertexArrayObject::sptr vao = ObjLoader::LoadFromFile(fileName);
	_vaosToSpawn.push_back(vao);
	//Adds material to list
	_materialsForSpawning = objMat;
	//Adds number to spawn for this object
	_numToSpawn.push_back(numToSpawn);

	coordinateX.push_back(_coordinateX);
	coordinateY.push_back(_coordinateY);


	//Adds the filename to the list
	_objectsToSpawn.push_back(fileName);
	//Sets it as not loaded
	_loadedIn.push_back(false);
}

void ChunkGenerator::AddProps(std::string fileName, ShaderMaterial::sptr objMat)
{
	VertexArrayObject::sptr vao = ObjLoader::LoadFromFile(fileName);
	_propVAOS.push_back(vao);
	_propMaterialsForSpawning.push_back(objMat);
	_propsToSpawn.push_back(fileName);
}

void ChunkGenerator::SpawnProp(glm::vec3 position, int spawnCount)
{
	std::vector<GameObject> tree;
	{
		if (!_loadedIn[1])
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile(_propsToSpawn[0]);
			_propVAOS.push_back(vao);
			_loadedIn[1] = true;
		}

		tree.push_back(Application::Instance().ActiveScene->CreateEntity(_propsToSpawn[0] + (std::to_string(spawnCount + 1))));
		tree[0].emplace<RendererComponent>().SetMesh(_propVAOS[0]).SetMaterial(_propMaterialsForSpawning[0]);
		tree[0].get<Transform>().SetLocalPosition(position);
	}

	_propsSpawned.push_back(tree);
}

void ChunkGenerator::RemoveObjectFromGeneration(std::string fileName)
{
	int index = Util::FindInVector(fileName, _objectsToSpawn);
	if (index == -1)
	{
		printf("Object not found in list\n");
		return;
	}

	//Erase from the vaosToSpawn, Materials, numbers, etc
	_vaosToSpawn.erase(_vaosToSpawn.begin() + index);
	_loadedIn.erase(_loadedIn.begin() + index);
	_materialsForSpawning.erase(_materialsForSpawning.begin() + index);
	_propMaterialsForSpawning.erase(_propMaterialsForSpawning.begin() + index);
	_numToSpawn.erase(_numToSpawn.begin() + index);
	coordinateX.erase(coordinateX.begin() + index);
	coordinateY.erase(coordinateY.begin() + index);
	_propVAOS.erase(_propVAOS.begin() + index);
	_propsToSpawn.erase(_propsToSpawn.begin() + index);
	//erase the filename from the list
	_objectsToSpawn.erase(_objectsToSpawn.begin() + index);
}

std::vector<std::string> ChunkGenerator::GetObjectsOnList()
{
	return _objectsToSpawn;
}
