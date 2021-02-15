//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <Behaviours/CameraControlBehaviour.h>
#include <Behaviours/RotateObjectBehaviour.h>

#include <IBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		/*Shader::sptr colourCorrectionShader = Shader::Create();
		colourCorrectionShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		colourCorrectionShader->LoadShaderPartFromFile("shaders/Post/colour_correction_frag.glsl", GL_FRAGMENT_SHADER);
		colourCorrectionShader->Link();*/

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 10.0f);
		glm::vec3 lightCol = glm::vec3(1.0f, 1.0f, 1.0f);
		float     lightAmbientPow = 0.05f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.2f;
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f;

		bool	  noLighting = true;
		bool	  ambientOnly = false;
		bool	  specularOnly = false;
		bool	  applyBloom = false;
		bool	  ambientAndSpecular = false;
		bool	  ambientSpecularAndBloom = false;
		bool	  toggleTextures = true;
		float	  bloomThreshold = 0.05f;
		int		  blurPasses = 10;

		bool	  useDiffuseRamp = false;
		bool	  useSpecularRamp = false;

		bool	  toggleWarmGrading = false;
		bool	  toggleCoolGrading = false;
		bool	  toggleCustomGrading = false;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		shader->SetUniform("u_NoLight", (int)noLighting);
		shader->SetUniform("u_AmbientOnly", (int)ambientOnly);
		shader->SetUniform("u_SpecularOnly", (int)specularOnly);
		shader->SetUniform("u_UseDiffuseRamp", (int)useDiffuseRamp);
		shader->SetUniform("u_UseSpecularRamp", (int)useSpecularRamp);

		PostEffect* basicEffect;

		int activeEffect = 3;
		std::vector<PostEffect*> effects;

		GreyscaleEffect* greyscaleEffect;

		SepiaEffect* sepiaEffect;

		ColourCorrectionEffect* colourCorrectionEffect;

		BloomEffect* bloomEffect;

		LUT3D warmCube("cubes/WarmCorrection.cube");
		LUT3D coolCube("cubes/CoolCorrection.cube");
		LUT3D customCube("cubes/CustomCorrection.cube");
		LUT3D neutralCube("cubes/Neutral.cube");

		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::Button("Regenerate Terrain", ImVec2(200.0f, 40.0f)))
			{
				ChunkGenerator::RegenerateEnvironment();
			}
			ImGui::Text("If there's any gaps, just keep Regenerating Terrain");
			if (ImGui::Checkbox("No Lighting", &noLighting)) {
				noLighting = true;
				ambientOnly = false;
				specularOnly = false;
				applyBloom = false;
				ambientAndSpecular = false;
				ambientSpecularAndBloom = false;
			}
			if (ImGui::Checkbox("Ambient Light Only", &ambientOnly)) {
				noLighting = false;
				ambientOnly = true;
				specularOnly = false;
				applyBloom = false;
				ambientAndSpecular = false;
				ambientSpecularAndBloom = false;
			}
			if (ImGui::Checkbox("Specular Light Only", &specularOnly)) {
				noLighting = false;
				ambientOnly = false;
				specularOnly = true;
				applyBloom = false;
				ambientAndSpecular = false;
				ambientSpecularAndBloom = false;
			}
			if (ImGui::Checkbox("Ambient and Specular Light", &ambientAndSpecular)) {
				noLighting = false;
				applyBloom = false;
				ambientSpecularAndBloom = false;
				if (ambientAndSpecular)
				{
					specularOnly = false;
					ambientOnly = false;
				}
				else
				{
					specularOnly = true;
					ambientOnly = true;
				}
			}
			if (ImGui::Checkbox("Ambient and Specular Light and Bloom", &ambientSpecularAndBloom)) {
				noLighting = false;
				ambientAndSpecular = false;
				toggleWarmGrading = false;
				toggleCoolGrading = false;
				toggleCustomGrading = false;
				activeEffect = 3;
				if (ambientSpecularAndBloom)
				{
					specularOnly = false;
					ambientOnly = false;
					ambientAndSpecular = false;
					applyBloom = true;
				}
				else
				{
					ambientAndSpecular = true;
					applyBloom = false;
				}
			}
			if (ImGui::Checkbox("Toggle Textures", &toggleTextures)) {

			}

			if (ambientSpecularAndBloom)
			{
				if (ImGui::SliderFloat("Bloom Threshold", &bloomThreshold, 0.01f, 1.0f))
				{
					bloomEffect->SetThreshold(bloomThreshold);
				}

				if (ImGui::SliderInt("Blur Passes", &blurPasses, 1, 10))
				{
					bloomEffect->SetPasses(blurPasses);
				}
			}

			shader->SetUniform("u_NoLight", (int)noLighting);
			shader->SetUniform("u_SpecularOnly", (int)specularOnly);
			shader->SetUniform("u_AmbientOnly", (int)ambientOnly);
			shader->SetUniform("u_UseTextures", (int)toggleTextures);
			bloomEffect->SetShaderUniform(3, "u_ApplyBloom", (int)applyBloom);
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		#pragma region TEXTURE LOADING

		// Load some textures from files
		Texture2D::sptr diffuse = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr diffuse2 = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr specular = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr reflectivity = Texture2D::LoadFromFile("images/box-reflections.bmp");

		// No Texture
		Texture2D::sptr noTexture = Texture2D::LoadFromFile("images/NoTexture.png");

		//Ramps
		Texture2D::sptr diffuseRamp = Texture2D::LoadFromFile("images/ramps/diffuseramp.png");
		Texture2D::sptr specularRamp = Texture2D::LoadFromFile("images/ramps/specularramp.png");

		// Lego Character Textures
		Texture2D::sptr minecraftGrass = Texture2D::LoadFromFile("images/GrassTexture.png");
		Texture2D::sptr minecraftSand = Texture2D::LoadFromFile("images/SandTexture.png");
		Texture2D::sptr minecraftSnow = Texture2D::LoadFromFile("images/SnowTexture.png");
		Texture2D::sptr minecraftTree = Texture2D::LoadFromFile("images/TreeTexture.png");
		Texture2D::sptr minecraftCloud = Texture2D::LoadFromFile("images/CloudTexture.png");

		//Specular Textures
		Texture2D::sptr nospecular = Texture2D::LoadFromFile("images/nospec.png");
		Texture2D::sptr darkspecular = Texture2D::LoadFromFile("images/DarkGrey.png");
		Texture2D::sptr offwhitespecular = Texture2D::LoadFromFile("images/offwhite.png");

		//Lego Block Colour Textures
		Texture2D::sptr legoblockred = Texture2D::LoadFromFile("images/Red.png");
		Texture2D::sptr legoblockbrown = Texture2D::LoadFromFile("images/Brown.png");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg");

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr material0 = ShaderMaterial::Create();
		material0->Shader = shader;
		material0->Set("s_Diffuse", diffuse);
		material0->Set("s_Diffuse2", diffuse2);
		material0->Set("s_Specular", specular);
		material0->Set("u_Shininess", 8.0f);
		material0->Set("u_TextureMix", 0.0f);
		material0->Set("s_DiffuseRamp", diffuseRamp);
		material0->Set("s_SpecularRamp", specularRamp);
		material0->Set("s_NoTextures", noTexture);

		// Minecraft Materials
		std::vector<ShaderMaterial::sptr> mincraftMaterials;
		ShaderMaterial::sptr GrassMaterial = ShaderMaterial::Create();
		GrassMaterial->Shader = shader;
		GrassMaterial->Set("s_Diffuse", minecraftGrass);
		GrassMaterial->Set("u_Shininess", 8.0f);
		GrassMaterial->Set("u_TextureMix", 0.0f);
		GrassMaterial->Set("s_DiffuseRamp", diffuseRamp);
		GrassMaterial->Set("s_SpecularRamp", specularRamp);
		GrassMaterial->Set("s_NoTextures", noTexture);
		mincraftMaterials.push_back(GrassMaterial);

		ShaderMaterial::sptr SandMaterial = ShaderMaterial::Create();
		SandMaterial->Shader = shader;
		SandMaterial->Set("s_Diffuse", minecraftSand);
		SandMaterial->Set("u_Shininess", 8.0f);
		SandMaterial->Set("u_TextureMix", 0.0f);
		SandMaterial->Set("s_DiffuseRamp", diffuseRamp);
		SandMaterial->Set("s_SpecularRamp", specularRamp);
		SandMaterial->Set("s_NoTextures", noTexture);
		mincraftMaterials.push_back(SandMaterial);

		ShaderMaterial::sptr SnowMaterial = ShaderMaterial::Create();
		SnowMaterial->Shader = shader;
		SnowMaterial->Set("s_Diffuse", minecraftSnow);
		SnowMaterial->Set("u_Shininess", 8.0f);
		SnowMaterial->Set("u_TextureMix", 0.0f);
		SnowMaterial->Set("s_DiffuseRamp", diffuseRamp);
		SnowMaterial->Set("s_SpecularRamp", specularRamp);
		SnowMaterial->Set("s_NoTextures", noTexture);
		mincraftMaterials.push_back(SnowMaterial);

		ShaderMaterial::sptr TreeMaterial = ShaderMaterial::Create();
		TreeMaterial->Shader = shader;
		TreeMaterial->Set("s_Diffuse", minecraftTree);
		TreeMaterial->Set("u_Shininess", 8.0f);
		TreeMaterial->Set("u_TextureMix", 0.0f);
		TreeMaterial->Set("s_DiffuseRamp", diffuseRamp);
		TreeMaterial->Set("s_SpecularRamp", specularRamp);
		TreeMaterial->Set("s_NoTextures", noTexture);

		ShaderMaterial::sptr CloudMaterial = ShaderMaterial::Create();
		CloudMaterial->Shader = shader;
		CloudMaterial->Set("s_Diffuse", minecraftCloud);
		CloudMaterial->Set("u_Shininess", 8.0f);
		CloudMaterial->Set("u_TextureMix", 0.0f);
		CloudMaterial->Set("s_DiffuseRamp", diffuseRamp);
		CloudMaterial->Set("s_SpecularRamp", specularRamp);
		CloudMaterial->Set("s_NoTextures", noTexture);

		// Load a second material for our reflective material!
		Shader::sptr reflectiveShader = Shader::Create();
		reflectiveShader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		reflectiveShader->LoadShaderPartFromFile("shaders/frag_reflection.frag.glsl", GL_FRAGMENT_SHADER);
		reflectiveShader->Link();

		Shader::sptr reflective = Shader::Create();
		reflective->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		reflective->LoadShaderPartFromFile("shaders/frag_blinn_phong_reflection.glsl", GL_FRAGMENT_SHADER);
		reflective->Link();

		// 
		ShaderMaterial::sptr material1 = ShaderMaterial::Create();
		material1->Shader = reflective;
		material1->Set("s_Diffuse", diffuse);
		material1->Set("s_Diffuse2", diffuse2);
		material1->Set("s_Specular", specular);
		material1->Set("s_Reflectivity", reflectivity);
		material1->Set("s_Environment", environmentMap);
		material1->Set("u_LightPos", lightPos);
		material1->Set("u_LightCol", lightCol);
		material1->Set("u_AmbientLightStrength", lightAmbientPow);
		material1->Set("u_SpecularLightStrength", lightSpecularPow);
		material1->Set("u_AmbientCol", ambientCol);
		material1->Set("u_AmbientStrength", ambientPow);
		material1->Set("u_LightAttenuationConstant", 1.0f);
		material1->Set("u_LightAttenuationLinear", lightLinearFalloff);
		material1->Set("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		material1->Set("u_Shininess", 8.0f);
		material1->Set("u_TextureMix", 0.5f);
		material1->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
		material1->Set("s_DiffuseRamp", diffuseRamp);
		material1->Set("s_SpecularRamp", specularRamp);
		material1->Set("s_NoTextures", noTexture);

		ShaderMaterial::sptr reflectiveMat = ShaderMaterial::Create();
		reflectiveMat->Shader = reflectiveShader;
		reflectiveMat->Set("s_Environment", environmentMap);
		reflectiveMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));

		int numChunks = 20;
		ChunkGenerator::AddObjectToGeneration("models/cube.obj", mincraftMaterials, numChunks, 0, 0);
		ChunkGenerator::AddObjectToGeneration("models/cube.obj", mincraftMaterials, numChunks, -1, 0);
		ChunkGenerator::AddObjectToGeneration("models/cube.obj", mincraftMaterials, numChunks, -1, 1);
		ChunkGenerator::AddObjectToGeneration("models/cube.obj", mincraftMaterials, numChunks, -2, 0);
		ChunkGenerator::AddObjectToGeneration("models/cube.obj", mincraftMaterials, numChunks, -2, 1);
		ChunkGenerator::AddObjectToGeneration("models/cube.obj", mincraftMaterials, numChunks, 0, 1);

		ChunkGenerator::AddProps("models/Tree.obj", TreeMaterial);
		ChunkGenerator::GenerateEnvironment();

		GameObject CloudObject1 = scene->CreateEntity("Cloud1");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Cloud1.obj");
			CloudObject1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(CloudMaterial);
			CloudObject1.get<Transform>().SetLocalPosition(0.0f, 0.0f, 40.0f);
			CloudObject1.get<Transform>().SetLocalRotation(0, 0, 90);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(CloudObject1);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 0.0f, 40.0f, 35.0f });
			pathing->Points.push_back({ 0.0f, 0.0f, 45.0f });
			pathing->Speed = 1.0f;
		}

		GameObject CloudObject2 = scene->CreateEntity("Cloud2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Cloud2.obj");
			CloudObject2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(CloudMaterial);
			CloudObject2.get<Transform>().SetLocalPosition(-20.0f, 20.0f, 40.0f);
			CloudObject2.get<Transform>().SetLocalRotation(0, 0, 90);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(CloudObject2);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -20.0f, 40.0f, 35.0f });
			pathing->Points.push_back({ -20.0f, 0.0f, 45.0f });
			pathing->Speed = 0.7f;
		}

		GameObject CloudObject3 = scene->CreateEntity("Cloud3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Cloud2.obj");
			CloudObject3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(CloudMaterial);
			CloudObject3.get<Transform>().SetLocalPosition(20.0f, 40.0f, 40.0f);
			CloudObject3.get<Transform>().SetLocalRotation(0, 0, 90);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(CloudObject3);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 20.0f, 40.0f, 35.0f });
			pathing->Points.push_back({ 20.0f, 0.0f, 45.0f });
			pathing->Speed = 0.3f;
		}

		GameObject CloudObject4 = scene->CreateEntity("Cloud4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Cloud1.obj");
			CloudObject4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(CloudMaterial);
			CloudObject4.get<Transform>().SetLocalPosition(-40.0f, 10.0f, 40.0f);
			CloudObject4.get<Transform>().SetLocalRotation(0, 0, 90);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(CloudObject3);
			// Set up a path for the object to follow
			pathing->Points.push_back({ -40.0f, 40.0f, 35.0f });
			pathing->Points.push_back({ -40.0f, 0.0f, 45.0f });
			pathing->Speed = 0.1f;
		}

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, -10, 20).LookAt(glm::vec3(0, 10, 20));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(3, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0, 10, 20));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		GameObject framebufferObject = scene->CreateEntity("Basic Buffer");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);

		GameObject SepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &SepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
		}
		effects.push_back(sepiaEffect);

		GameObject ColourCorrectionEffectObject = scene->CreateEntity("Colour Correction Effect");
		{
			colourCorrectionEffect = &ColourCorrectionEffectObject.emplace<ColourCorrectionEffect>();
			colourCorrectionEffect->Init(width, height);
		}
		effects.push_back(colourCorrectionEffect);

		GameObject BloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			bloomEffect = &BloomEffectObject.emplace<BloomEffect>();
			bloomEffect->Init(width, height);
		}
		effects.push_back(bloomEffect);

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			/*controllables.push_back(obj2);
			controllables.push_back(obj3);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});*/
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			basicEffect->Clear();
			//colourCorrection->Clear();

			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}

			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});
			
			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;
						
			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;
				
				return false;
			});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			basicEffect->BindBuffer(0);
			//colourCorrection->Bind();

			// Iterate over the render group components and draw them
			renderGroup.each( [&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});

			/*colourCorrection->Unbind();

			colourCorrectionShader->Bind();
			colourCorrection->BindColorAsTexture(0, 0);
			testCube.bind(30);
			colourCorrection->DrawFullscreenQuad();
			testCube.unbind(30);
			colourCorrection->UnbindTexture(0);
			colourCorrectionShader->UnBind();*/

			basicEffect->UnbindBuffer();

			effects[activeEffect]->ApplyEffect(basicEffect);
			effects[activeEffect]->DrawToScreen();

			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		ChunkGenerator::CleanUpPointers();
		BackendHandler::ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}