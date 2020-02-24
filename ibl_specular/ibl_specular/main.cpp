//gl
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include "Environment.h"

using namespace std;

GLFWwindow *g_Window = 0;
// const
const int g_WindowWidth = 1280;
const int g_WindowHeight = 720;
// light
glm::vec3 g_LightPosition(0.0f, 0.0f, 5.0f);
float g_LightColor[3] = { 1.0, 1.0, 1.0 };

GLint g_TextureBrdfLUT = -1;

// model
Model *g_LightModel = NULL;

// 唯一名字和imgui界面信息的map,用于模型/材质/环境的界面,便于增加
map<string, Environment*> g_EnvironmentMap;
map<string, PBRMaterial*> g_PBRMaterialMap;

map<string, Model*> g_ModelMap;

string g_CurrentModelName;
string g_CurrentPBRMaterialName;
string g_CurrentEnvironmentName;

// PBR 配置开关
float testAlbedo[3] = { 1,1,1 };
float testMetallic = 1;
float testRoughness = 1;
bool testHaveSpecular = 1;
bool g_UseIBL = 1;
bool g_UseEmissive = 1;
bool g_UseLightDistanceAttenuation = 0;
bool g_UseGamma = 1;
bool g_UseHDR = 1;
bool g_UseAO = 1;
bool g_UsePBR = 1;
bool g_SkyBox = 1;
bool g_DirectLight = 1;
bool g_GlobalLight = 1;
// 显示选择材质开关
bool g_ChoosePBRMaterial = false;
// 自动旋转开关
bool g_ModelAutoRotated = false;
float g_ModelRotatedAngle = 0.0f;
bool g_LightAutoRotated = false;
// imgui按钮变色
ImVec4 g_ButtonColor;

// camera
Camera  g_Camera(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90, 0);
bool g_Keys[1024];
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);

bool initWindow();
bool initGLAD();
bool initImgui();

void bindTexture(GLenum textureUnit, GLenum textureType, GLuint texture);
GLuint createCubemapTexture(vector<std::string> faces);
GLuint equirectangular_2_cubemap(Shader convertShader, GLuint srcTexture, int width, int height);

Model* createModel(string name);

void chooseModel(int i);

void renderImgui();

// imgui
void setButtonColor(ImVec4 btnAfterClickColor);
void revertButtonColor();

void releaseResource();

// assimp多次加载blend格式文件有问题：第二次加载程序一直在循环输出错误
// 材质/天空盒预览图
// 未优化：添加新的模型/材质/天空盒过于繁琐

// Ogre不支持实时预计算

// 拆分反射率方程diffuse(diffuse Li) + specular(specular Li, BRDF)
//
// 漫反射幅度照diffuse Li中存着p点半球领域的颜色(漫反射幅度值)[半球领域上所有的点对p点的影响],只和入射方向有关,和出射方向无关
// 和出射方向无关:无论从哪个方向看p点，p点的漫反射幅度值始终不变
// 因为漫反射的光照分布是均匀，所以在半球空间均匀采样就可以
int main()
{
	// 初始化环境
	{
		if (!initWindow())
			return -1;
		if (!initGLAD())
			return -1;
		if (!initImgui())
			return -1;
	}

	// 界面数据
	{
		// 环境/天空盒
		{
			// 停机场_下午/Helipad_Afternoon
			{
				EnvironmentTextureInfo preInfo;
				preInfo.path = "../../resource/hdr/Helipad_Afternoon/LA_Downtown_Afternoon_preview.jpg";
				EnvironmentTextureInfo envInfo;
				envInfo.path = "../../resource/hdr/Helipad_Afternoon/LA_Downtown_Afternoon_Fishing_B_8k.jpg";
				envInfo.flip = true;
				EnvironmentTextureInfo diffInfo;
				diffInfo.path = "../../resource/hdr/Helipad_Afternoon/LA_Downtown_Afternoon_Fishing_Env.hdr";
				diffInfo.hdr = true;
				diffInfo.flip = true;
				EnvironmentTextureInfo specInfo;
				specInfo.path = "../../resource/hdr/Helipad_Afternoon/LA_Downtown_Afternoon_Fishing_3k.hdr";
				specInfo.hdr = true;
				specInfo.flip = true;

				Environment *env = new Environment;
				env->showName = u8"停机场_下午/Helipad_Afternoon";
				env->previewTexInfo = preInfo;
				env->environmentTexInfo = envInfo;
				env->diffuseTexInfo = diffInfo;
				env->specularTexInfo = specInfo;

				g_EnvironmentMap.insert(pair<string, Environment*>("Helipad_Afternoon", env));
			}
			// 造纸厂废墟E/PaperMill_Ruins_E
			{
				EnvironmentTextureInfo preInfo;
				preInfo.path = "../../resource/hdr/PaperMill_Ruins_E/PaperMill_E_preview.jpg";
				EnvironmentTextureInfo envInfo;
				//envInfo.path = "../../resource/hdr/PaperMill_Ruins_E/PaperMill_E_8k.jpg";
				envInfo.path = "../../resource/hdr/PaperMill_Ruins_E/PaperMill_E_3k.hdr";
				envInfo.hdr = true;
				envInfo.flip = true;
				EnvironmentTextureInfo diffInfo;
				diffInfo.path = "../../resource/hdr/PaperMill_Ruins_E/PaperMill_E_Env.hdr";
				diffInfo.hdr = true;
				diffInfo.flip = true;
				EnvironmentTextureInfo specInfo;
				specInfo.path = "../../resource/hdr/PaperMill_Ruins_E/PaperMill_E_3k.hdr";
				specInfo.hdr = true;
				specInfo.flip = true;

				Environment *env = new Environment;
				env->showName = u8"造纸厂废墟E/PaperMill_Ruins_E";
				env->previewTexInfo = preInfo;
				env->environmentTexInfo = envInfo;
				env->diffuseTexInfo = diffInfo;
				env->specularTexInfo = specInfo;

				g_EnvironmentMap.insert(pair<string, Environment*>("PaperMill_Ruins_E", env));
			}
			// 银河/Milkyway
			{
				EnvironmentTextureInfo preInfo;
				preInfo.path = "../../resource/hdr/Milkyway/Milkyway_preview.jpg";
				EnvironmentTextureInfo envInfo;
				envInfo.path = "../../resource/hdr/Milkyway/Milkyway_BG.jpg";
				envInfo.flip = true;
				EnvironmentTextureInfo diffInfo;
				diffInfo.path = "../../resource/hdr/Milkyway/Milkyway_Light.hdr";
				diffInfo.hdr = true;
				diffInfo.flip = true;
				EnvironmentTextureInfo specInfo;
				specInfo.path = "../../resource/hdr/Milkyway/Milkyway_small.hdr";
				specInfo.hdr = true;
				specInfo.flip = true;

				Environment *env = new Environment;
				env->showName = u8"银河/Milkyway";
				env->previewTexInfo = preInfo;
				env->environmentTexInfo = envInfo;
				env->diffuseTexInfo = diffInfo;
				env->specularTexInfo = specInfo;

				g_EnvironmentMap.insert(pair<string, Environment*>("Milkyway", env));
			}
			// 冬季森林/Winter_Forest
			{
				EnvironmentTextureInfo preInfo;
				preInfo.path = "../../resource/hdr/Winter_Forest/WinterForest_preview.jpg";
				EnvironmentTextureInfo envInfo;
				envInfo.path = "../../resource/hdr/Winter_Forest/WinterForest_8k.jpg";
				envInfo.flip = true;
				EnvironmentTextureInfo diffInfo;
				diffInfo.path = "../../resource/hdr/Winter_Forest/WinterForest_Env.hdr";
				diffInfo.hdr = true;
				diffInfo.flip = true;
				EnvironmentTextureInfo specInfo;
				specInfo.path = "../../resource/hdr/Winter_Forest/WinterForest_Ref.hdr";
				specInfo.hdr = true;
				specInfo.flip = true;

				Environment *env = new Environment;
				env->showName = u8"冬季森林/Winter_Forest";
				env->previewTexInfo = preInfo;
				env->environmentTexInfo = envInfo;
				env->diffuseTexInfo = diffInfo;
				env->specularTexInfo = specInfo;

				g_EnvironmentMap.insert(pair<string, Environment*>("Winter_Forest", env));
			}
			// 沙漠日落/Zion_Sunsetpeek
			{
				EnvironmentTextureInfo preInfo;
				preInfo.path = "../../resource/hdr/Zion_Sunsetpeek/Zion_7_Sunsetpeek_preview.jpg";
				EnvironmentTextureInfo envInfo;
				envInfo.path = "../../resource/hdr/Zion_Sunsetpeek/Zion_7_Sunsetpeek_8k_Bg.jpg";
				envInfo.flip = true;
				EnvironmentTextureInfo diffInfo;
				diffInfo.path = "../../resource/hdr/Zion_Sunsetpeek/Zion_7_Sunsetpeek_Env.hdr";
				diffInfo.hdr = true;
				diffInfo.flip = true;
				EnvironmentTextureInfo specInfo;
				specInfo.path = "../../resource/hdr/Zion_Sunsetpeek/Zion_7_Sunsetpeek_Ref.hdr";
				specInfo.hdr = true;
				specInfo.flip = true;

				Environment *env = new Environment;
				env->showName = u8"沙漠日落/Zion_Sunsetpeek";
				env->previewTexInfo = preInfo;
				env->environmentTexInfo = envInfo;
				env->diffuseTexInfo = diffInfo;
				env->specularTexInfo = specInfo;

				g_EnvironmentMap.insert(pair<string, Environment*>("Zion_Sunsetpeek", env));
			}
		}
		// 加载 环境/天空盒 的预览图
		for (auto it = g_EnvironmentMap.begin(); it != g_EnvironmentMap.end(); it++)
		{
			Environment *env = it->second;
			env->loadPreviewTexture();
		}

		// 材质
		{
			// Gold
			{
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/gold/normal.png");
				mat->pathVec.push_back("../../resource/pbr/gold/albedo.png");
				mat->pathVec.push_back("../../resource/pbr/gold/metallic.png");
				mat->pathVec.push_back("../../resource/pbr/gold/roughness.png");
				mat->pathVec.push_back("../../resource/pbr/gold/ao.png");
				mat->pathVec.push_back("");

				mat->previewPath = "../../resource/pbr/gold/albedo.png";

				mat->showName = u8"金子/gold";
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 0;
				mat->g_MergeMR = 0;

				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("Gold", mat));
			}
			// Plastic
			{
				// normal,albedo,metallic,roughness,ao,emissive
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/plastic/normal.png");
				mat->pathVec.push_back("../../resource/pbr/plastic/albedo.png");
				mat->pathVec.push_back("../../resource/pbr/plastic/metallic.png");
				mat->pathVec.push_back("../../resource/pbr/plastic/roughness.png");
				mat->pathVec.push_back("../../resource/pbr/plastic/ao.png");
				mat->pathVec.push_back("");

				mat->previewPath = "../../resource/pbr/plastic/albedo.png";

				mat->showName = u8"塑料/plastic";
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 0;
				mat->g_MergeMR = 0;
				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("Plastic", mat));
			}
			// MetallicGrid
			{
				// normal,albedo,metallic,roughness,ao,emissive
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/metallic_grid/metalgrid1_normal-ogl.png");
				mat->pathVec.push_back("../../resource/pbr/metallic_grid/metalgrid1_basecolor.png");
				mat->pathVec.push_back("../../resource/pbr/metallic_grid/metalgrid1_metallic.png");
				mat->pathVec.push_back("../../resource/pbr/metallic_grid/metalgrid1_roughness.png");
				mat->pathVec.push_back("../../resource/pbr/metallic_grid/metalgrid1_AO.png");
				mat->pathVec.push_back("");

				mat->previewPath = "../../resource/pbr/metallic_grid/metalgrid1_basecolor.png";

				mat->showName = u8"金属网格/metallic_grid";
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 0;
				mat->g_MergeMR = 0;
				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("MetallicGrid", mat));
			}
			// Wood
			{
				// normal,albedo,metallic,roughness,ao,emissive
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/wood/bamboo-wood-semigloss-normal.png");
				mat->pathVec.push_back("../../resource/pbr/wood/bamboo-wood-semigloss-albedo.png");
				mat->pathVec.push_back("../../resource/pbr/wood/bamboo-wood-semigloss-metal.png");
				mat->pathVec.push_back("../../resource/pbr/wood/bamboo-wood-semigloss-roughness.png");
				mat->pathVec.push_back("../../resource/pbr/wood/bamboo-wood-semigloss-ao.png");
				mat->pathVec.push_back("");

				mat->previewPath = "../../resource/pbr/wood/bamboo-wood-semigloss-albedo.png";

				mat->showName = u8"木材/wood";
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 0;
				mat->g_MergeMR = 0;
				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("Wood", mat));
			}
			// Rock
			{
				// normal,albedo,metallic,roughness,ao,emissive
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/rock/rock_sliced_Normal.png");
				mat->pathVec.push_back("../../resource/pbr/rock/rock_sliced_Base_Color.png");
				mat->pathVec.push_back("../../resource/pbr/rock/rock_sliced_Metallic.png");
				mat->pathVec.push_back("../../resource/pbr/rock/rock_sliced_Height.png");
				mat->pathVec.push_back("../../resource/pbr/rock/rock_sliced_Ambient_Occlusion.png");
				mat->pathVec.push_back("");

				mat->previewPath = "../../resource/pbr/rock/rock_sliced_Base_Color.png";

				mat->showName = u8"岩石/rock";
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 0;
				mat->g_MergeMR = 0;
				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("Rock", mat));
			}
			// RustedIron
			{
				// normal,albedo,metallic,roughness,ao,emissive
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/rusted_iron/normal.png");
				mat->pathVec.push_back("../../resource/pbr/rusted_iron/albedo.png");
				mat->pathVec.push_back("../../resource/pbr/rusted_iron/metallic.png");
				mat->pathVec.push_back("../../resource/pbr/rusted_iron/roughness.png");
				mat->pathVec.push_back("../../resource/pbr/rusted_iron/ao.png");
				mat->pathVec.push_back("");

				mat->previewPath = "../../resource/pbr/rusted_iron/albedo.png";

				mat->showName = u8"铁锈/rusted_iron";
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 0;
				mat->g_MergeMR = 0;
				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("RustedIron", mat));
			}
			// Wall
			{
				// normal,albedo,metallic,roughness,ao,emissive
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/wall/normal.png");
				mat->pathVec.push_back("../../resource/pbr/wall/albedo.png");
				mat->pathVec.push_back("../../resource/pbr/wall/metallic.png");
				mat->pathVec.push_back("../../resource/pbr/wall/roughness.png");
				mat->pathVec.push_back("../../resource/pbr/wall/ao.png");
				mat->pathVec.push_back("");

				mat->previewPath = "../../resource/pbr/wall/albedo.png";

				mat->showName = u8"墙壁/wall";
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 0;
				mat->g_MergeMR = 0;
				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("Wall", mat));
			}
			// DamagedHelmet
			{
				// normal,albedo,metallic,roughness,ao,emissive
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/DamagedHelmet/Default_normal.jpg");
				mat->pathVec.push_back("../../resource/pbr/DamagedHelmet/Default_albedo.jpg");
				mat->pathVec.push_back("../../resource/pbr/DamagedHelmet/Default_metalRoughness.jpg");
				mat->pathVec.push_back("");
				mat->pathVec.push_back("../../resource/pbr/DamagedHelmet/Default_AO.jpg");
				mat->pathVec.push_back("../../resource/pbr/DamagedHelmet/Default_emissive.jpg");

				mat->isShow = false;
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 1;
				mat->g_MergeMR = 1;
				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("DamagedHelmet", mat));
			}
			// Cerberus
			{
				// normal,albedo,metallic,roughness,ao,emissive
				PBRMaterial *mat = new PBRMaterial;
				mat->pathVec.push_back("../../resource/pbr/Cerberus_by_Andrew_Maximov/Textures/Cerberus_N.tga");
				mat->pathVec.push_back("../../resource/pbr/Cerberus_by_Andrew_Maximov/Textures/Cerberus_A.tga");
				mat->pathVec.push_back("../../resource/pbr/Cerberus_by_Andrew_Maximov/Textures/Cerberus_M.tga");
				mat->pathVec.push_back("../../resource/pbr/Cerberus_by_Andrew_Maximov/Textures/Cerberus_R.tga");
				mat->pathVec.push_back("../../resource/pbr/Cerberus_by_Andrew_Maximov/Textures/Raw/Cerberus_AO.tga");
				mat->pathVec.push_back("");

				mat->isShow = false;
				mat->g_MergeMR = 0;
				mat->g_HaveAO = 1;
				mat->g_HaveEmissive = 0;
				g_PBRMaterialMap.insert(pair<string, PBRMaterial*>("Cerberus", mat));
			}
		}
		// 加载 材质 的预览图
		for (auto it = g_PBRMaterialMap.begin(); it != g_PBRMaterialMap.end(); it++)
		{
			PBRMaterial *mat = it->second;
			mat->loadPreviewTexture();
		}
	}

	// 设置默认模型/材质/环境
	g_CurrentModelName = "DamagedHelmet";
	g_CurrentPBRMaterialName = "DamagedHelmet";
	g_CurrentEnvironmentName = "PaperMill_Ruins_E";
	//g_CurrentModelName = "Sphere";
	//g_CurrentPBRMaterialName = "Gold";

	// 间接光BRDF预计算
	g_TextureBrdfLUT = createTexture("../../resource/brdfLUT.png", false, true).id;

	Shader pbrShader("./pbr.vs", "./pbr_ogre.fs");
	Shader lightShader("./Quad.vs", "./Quad.fs");
	Shader skyBoxShader("./SkyBox.vs", "./SkyBox.fs");
	Shader equirectangularToCubemapShader("./SkyBox.vs", "./equirectangular_to_cubemap.fs");
	Shader prefilterShader("./SkyBox.vs", "./2.2.2.prefilter.fs");
	//Shader equirectangularToCubemapShader("./SkyBox.vs", "./equirectangular_to_cubemap.fs");

	while (!glfwWindowShouldClose(g_Window))
	{
		// 加载当前选中的模型/材质/环境
		Model *pbrModel = NULL;
		PBRMaterial *pbrMaterial = NULL;
		Environment *environment = NULL;

		auto modelIt = g_ModelMap.find(g_CurrentModelName);
		if (modelIt != g_ModelMap.end())
			pbrModel = modelIt->second;
		else
			pbrModel = createModel(g_CurrentModelName);

		auto pbrMaterialIt = g_PBRMaterialMap.find(g_CurrentPBRMaterialName);
		if (pbrMaterialIt != g_PBRMaterialMap.end())
		{
			pbrMaterial = pbrMaterialIt->second;
			pbrMaterial->load();
		}

		auto environmentIt = g_EnvironmentMap.find(g_CurrentEnvironmentName);
		if (environmentIt != g_EnvironmentMap.end())
		{
			environment = environmentIt->second;
			environment->load(equirectangularToCubemapShader, prefilterShader);
		}

		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//event
		glfwPollEvents();
		processInput(g_Window);

		// imgui
		if(true)
		{
			char window_title[50];
			sprintf_s(window_title, "PBR: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			glfwSetWindowTitle(g_Window, window_title);

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::Begin("Param");
			{
				ImGui::SliderFloat("metallic", &testMetallic, 0.0f, 1.0f, "%.3f");            // Edit 1 float using a slider from 0.0f to 1.0f
				ImGui::SliderFloat("roughness", &testRoughness, 0.0f, 1.0f, "%.3f");
				ImGui::ColorEdit3("albedo(Surface Color)", (float*)&testAlbedo);
				ImGui::Checkbox(u8"Specular", &testHaveSpecular);
				//ImGui::SliderFloat("albedo", &g_Albedo, 0.0f, 1.0f, "%.3f");
				//ImGui::InputFloat3(u8"light color", g_LightColor);
				//ImGui::InputFloat3(u8"albedo(Surface Color)", g_Albedo);

				ImGui::Checkbox(u8"ModelAutoRotated", &g_ModelAutoRotated);
				ImGui::Checkbox(u8"LightAutoRotated", &g_LightAutoRotated);
				setButtonColor(ImVec4(255, 0, 244, 102));
				if (ImGui::Button(u8"CameraReset"))
					g_Camera.reset();
				revertButtonColor();
				if (pbrMaterial->g_HaveEmissive)
					ImGui::Checkbox(u8"自发光/Emissive", &g_UseEmissive);
				if (pbrMaterial->g_HaveAO)
					ImGui::Checkbox(u8"环境光遮蔽/AO", &g_UseAO);
				ImGui::Checkbox(u8"IBL", &g_UseIBL);
				ImGui::Checkbox(u8"随光源距离衰减/LightDistanceAttenuation", &g_UseLightDistanceAttenuation);
				ImGui::Checkbox(u8"Gamma", &g_UseGamma);
				ImGui::Checkbox(u8"HDR", &g_UseHDR);
				ImGui::Checkbox(u8"天空盒/Skybox", &g_SkyBox);
				ImGui::Checkbox(u8"直接光/DirectLight", &g_DirectLight);
				ImGui::Checkbox(u8"间接光/全局光/Global", &g_GlobalLight);
				//ImGui::Checkbox(u8"PBR", &g_UsePBR);

				ImGui::ColorEdit3(u8"light color", (float*)&g_LightColor);

				if (ImGui::Button(u8"摄像头/ShaderBall"))
					chooseModel(0); ImGui::SameLine();
				if (ImGui::Button(u8"破损头盔/DamagedHelmet"))
					chooseModel(1);
				if (ImGui::Button(u8"枪/Cerberus"))
					chooseModel(2); ImGui::SameLine();
				if (ImGui::Button(u8"材质球/Sphere"))
					chooseModel(3);
			}
			ImGui::End();

			ImGui::Begin("Material");
			if (g_ChoosePBRMaterial)
			{
				int i = 1;
				for (auto it = g_PBRMaterialMap.begin(); it != g_PBRMaterialMap.end(); it++)
				{
					string name = it->first;
					PBRMaterial *mat = it->second;
					if (mat->isShow)
					{
						//if (ImGui::Button(env->showName.c_str()))
						//	g_CurrentPBRMaterialName = name;

						ImTextureID my_tex_id = (void*)mat->previewTexInfo.id;
						if (ImGui::ImageButton(my_tex_id, ImVec2(55, 55), ImVec2(0, 0), ImVec2(1, 1), 1, ImColor(0, 0, 0, 0)))
							g_CurrentPBRMaterialName = name;

						if (i % 6 != 0)
							ImGui::SameLine();

						i++;
					}
				}
			}
			ImGui::End();

			ImGui::Begin("Environment");
			for (auto it = g_EnvironmentMap.begin(); it != g_EnvironmentMap.end(); it++)
			{
				string name = it->first;
				Environment *env = it->second;
				//if (ImGui::Button(env->showName.c_str()))
				//	g_CurrentEnvironmentName = name;

				ImTextureID my_tex_id = (void*)env->previewTexInfo.texId;
				if (ImGui::ImageButton(my_tex_id, ImVec2(240, 120), ImVec2(0, 0), ImVec2(1, 1), 1, ImColor(0, 0, 0, 0)))
					g_CurrentEnvironmentName = name;
			}
			ImGui::End();

			//static bool showDemo = true;
			//ImGui::ShowDemoWindow(&showDemo);
		}
		  
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, g_WindowWidth, g_WindowHeight);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		// 渲染PBR模型
		if(true)
		{
			pbrShader.use();
			glm::mat4 model;

			if (g_ModelAutoRotated)
				g_ModelRotatedAngle += 0.3f;
			model = glm::rotate(model, glm::radians(g_ModelRotatedAngle), glm::vec3(0, 1, 0));
			//model = glm::rotate(model, glm::radians((float)glfwGetTime() * 30.0f), glm::vec3(0, 1, 0));
			model = glm::scale(model, pbrModel->g_ModelScale);
			model = glm::rotate(model, glm::radians(pbrModel->g_ModelRotateZ), glm::vec3(0, 0, 1));
			model = glm::rotate(model, glm::radians(pbrModel->g_ModelRotateY), glm::vec3(0, 1, 0));
			model = glm::rotate(model, glm::radians(pbrModel->g_ModelRotateX), glm::vec3(1, 0, 0));
			
			glm::mat4 view = g_Camera.GetViewMatrix();
			glm::mat4 projection = glm::perspective(glm::radians(g_Camera.Zoom), (GLfloat)g_WindowWidth / (GLfloat)g_WindowHeight, 0.1f, 1000.0f);
			pbrShader.setMat4("model", model);
			pbrShader.setMat4("view", view);
			pbrShader.setMat4("projection", projection);

			pbrShader.setVec3("u_lightPosition", g_LightPosition);
			pbrShader.setVec3("u_lightColor", glm::vec3(g_LightColor[0], g_LightColor[1], g_LightColor[2]));
			pbrShader.setVec3("u_cameraPosition", g_Camera.Position);

			pbrShader.setBool("u_mergeMR", pbrMaterial->g_MergeMR);
			pbrShader.setBool("u_haveEmissive", pbrMaterial->g_HaveEmissive);
			pbrShader.setBool("u_haveAO", pbrMaterial->g_HaveAO);

			pbrShader.setBool("u_useIBL", g_UseIBL);
			pbrShader.setBool("u_useEmissive", g_UseEmissive);
			pbrShader.setBool("u_useLightDistanceAttenuation", g_UseLightDistanceAttenuation);
			pbrShader.setBool("u_useGamma", g_UseGamma);
			pbrShader.setBool("u_useHDR", g_UseHDR);
			pbrShader.setBool("u_useAO", g_UseAO);
			pbrShader.setBool("u_usePBR", g_UsePBR);
			pbrShader.setBool("u_useDirectLight", g_DirectLight);
			pbrShader.setBool("u_useGlobalLight", g_GlobalLight);

			pbrShader.setVec3("u_testAlbedo", glm::vec3(testAlbedo[0], testAlbedo[1], testAlbedo[2]));
			pbrShader.setFloat("u_testMetallic", testMetallic);
			pbrShader.setFloat("u_testRoughness", testRoughness);
			pbrShader.setBool("u_testHaveSpecular", testHaveSpecular);

			pbrShader.setInt("u_normalTexture", 0);
			pbrShader.setInt("u_albedoTexture", 1);
			pbrShader.setInt("u_metallicTexture", 2);
			pbrShader.setInt("u_roughnessTexture", 3);
			pbrShader.setInt("u_aoTexture", 4);
			pbrShader.setInt("u_emissiveTexture", 5);
			pbrShader.setInt("u_diffuseEnvSampler", 6);
			pbrShader.setInt("u_specularEnvSampler", 7);
			pbrShader.setInt("u_brdfLUT", 8);

			// normal,albedo,metallic,roughness,ao,emissive
			bindTexture(GL_TEXTURE0, GL_TEXTURE_2D, pbrMaterial->texVec[0]);
			bindTexture(GL_TEXTURE1, GL_TEXTURE_2D, pbrMaterial->texVec[1]);
			bindTexture(GL_TEXTURE2, GL_TEXTURE_2D, pbrMaterial->texVec[2]);
			bindTexture(GL_TEXTURE3, GL_TEXTURE_2D, pbrMaterial->texVec[3]);
			bindTexture(GL_TEXTURE4, GL_TEXTURE_2D, pbrMaterial->texVec[4]);
			bindTexture(GL_TEXTURE5, GL_TEXTURE_2D, pbrMaterial->texVec[5]);
			bindTexture(GL_TEXTURE6, GL_TEXTURE_CUBE_MAP, environment->diffuseTexInfo.cubemapId);
			bindTexture(GL_TEXTURE7, GL_TEXTURE_CUBE_MAP, environment->specularTexInfo.cubemapId);
			bindTexture(GL_TEXTURE8, GL_TEXTURE_2D, g_TextureBrdfLUT);

			pbrModel->Draw(pbrShader);
		}

		// 渲染灯
		if (true)
		{
			if (!g_LightModel)
			{
				g_LightModel = new Model;
				g_LightModel->Load("../../3dModel/Lightbulb.obj");
			}

			lightShader.use();
			glm::mat4 model;
			if (g_LightAutoRotated)
			{
				glm::mat4 lightRotatedMat;
				lightRotatedMat = glm::rotate(lightRotatedMat, glm::radians(0.3f), glm::vec3(1, 0, 0));
				g_LightPosition = glm::mat3(lightRotatedMat) * g_LightPosition;
			}
			//model = glm::rotate(model, glm::radians(g_LightRotatedAngle), glm::vec3(1, 0, 0));
			model = glm::translate(model, g_LightPosition);
			model = glm::scale(model, glm::vec3(0.1));
			//model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 1, 0));
			model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
			glm::mat4 view = g_Camera.GetViewMatrix();
			glm::mat4 projection = glm::perspective(glm::radians(g_Camera.Zoom), (GLfloat)g_WindowWidth / (GLfloat)g_WindowHeight, 0.1f, 1000.0f);
			lightShader.setMat4("model", model);
			lightShader.setMat4("view", view);
			lightShader.setMat4("projection", projection);

			g_LightModel->Draw(lightShader);
		}

		// 渲染天空盒
		if(g_SkyBox)
		{
			// draw skybox as last
			glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
			skyBoxShader.use();
			glm::mat4 view = glm::mat4(glm::mat3(g_Camera.GetViewMatrix())); // remove translation from the view matrix
			skyBoxShader.setMat4("view", view);
			glm::mat4 projection = glm::perspective(glm::radians(g_Camera.Zoom), (GLfloat)g_WindowWidth / (GLfloat)g_WindowHeight, 0.1f, 1000.0f);
			skyBoxShader.setMat4("projection", projection);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, environment->environmentTexInfo.cubemapId);
			renderCube();
			glDepthFunc(GL_LESS); // set depth function back to default
		}

		//imgui
		renderImgui();

		glfwSwapBuffers(g_Window);
	}

	releaseResource();

	//release glfw
	glfwTerminate();

	return 0;
}

void chooseModel(int i)
{
	g_ChoosePBRMaterial = false;
	switch (i)
	{
	case 0:
	{
		g_CurrentModelName = "ShaderBall";
		g_CurrentPBRMaterialName = "Gold";
		g_ChoosePBRMaterial = true;
	}
	break;
	case 1:
	{
		g_CurrentModelName = "DamagedHelmet";
		g_CurrentPBRMaterialName = "DamagedHelmet";
	}
	break;
	case 2:
	{
		g_CurrentModelName = "Cerberus";
		g_CurrentPBRMaterialName = "Cerberus";
	}
	break;
	case 3:
	{
		g_CurrentModelName = "Sphere";
		g_CurrentPBRMaterialName = "Gold";
		g_ChoosePBRMaterial = true;
	}
	break;
	default:
		break;
	}
}

Model* createModel(string name)
{
	Model *model = NULL;

	if (name == "ShaderBall")
	{
		model = new Model;
		model->g_ModelScale = glm::vec3(0.003f);
		model->g_ModelRotateX = 0.0f;
		model->g_ModelRotateY = 0.0f;
		model->g_ModelRotateZ = 0.0f;
		model->Load("../../3dModel/shaderBall.obj");
	}

	if (name == "Sphere")
	{
		model = new Model;
		model->g_ModelScale = glm::vec3(1.0f);
		model->g_ModelRotateX = 0.0f;
		model->g_ModelRotateY = 0.0f;
		model->g_ModelRotateZ = 0.0f;
		model->Load("../../3dModel/Sphere.obj");
	}

	if (name == "DamagedHelmet")
	{
		model = new Model;
		model->g_ModelScale = glm::vec3(1.0f);
		model->g_ModelRotateX = -90.0f;
		model->g_ModelRotateY = 0.0f;
		model->g_ModelRotateZ = 0.0f;
		model->Load("../../3dModel/DamagedHelmet.blend");
	}

	if (name == "Cerberus")
	{
		model = new Model;
		model->g_ModelScale = glm::vec3(0.03f);
		model->g_ModelRotateX = 0.0f;
		model->g_ModelRotateY = 90.0f;
		model->g_ModelRotateZ = 0.0f;
		model->Load("../../3dModel/Cerberus_LP.FBX");
	}

	g_ModelMap.insert(pair<string, Model*>(name, model));

	return model;
}

bool initGLAD()
{
	//init glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		return false;

	return true;
}
bool initImgui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
	std::string fontDir(getExeDir() + "\\simhei.ttf");
	io.Fonts->AddFontFromFileTTF(fontDir.c_str(), 15.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

	// Setup Dear ImGui style
	ImGuiStyle style = ImGui::GetStyle();
	g_ButtonColor = style.Colors[21];
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
	const char* glsl_version = "#version 330 core";
	ImGui_ImplOpenGL3_Init(glsl_version);

	return true;
}
bool initWindow()
{
	//init glfw
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	g_Window = glfwCreateWindow(g_WindowWidth, g_WindowHeight, "PBR&IBL", NULL, NULL);
	if (g_Window == NULL)
	{
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(g_Window);

	//registe callback
	glfwSetFramebufferSizeCallback(g_Window, framebufferSizeCallback);
	glfwSetCursorPosCallback(g_Window, mouseCallback);
	glfwSetScrollCallback(g_Window, scrollCallback);
	glfwSetKeyCallback(g_Window, keyCallback);

	return true;
}

//callback
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			g_Keys[key] = true;
		else if (action == GLFW_RELEASE)
			g_Keys[key] = false;
	}
}
void processInput(GLFWwindow *window)
{
	float speed = 5;

	if (g_Keys[GLFW_KEY_W] && g_Keys[GLFW_KEY_LEFT_SHIFT])
		g_Camera.ProcessKeyboard(FORWARD, deltaTime * speed);
	if (g_Keys[GLFW_KEY_S] && g_Keys[GLFW_KEY_LEFT_SHIFT])
		g_Camera.ProcessKeyboard(BACKWARD, deltaTime * speed);
	if (g_Keys[GLFW_KEY_A] && g_Keys[GLFW_KEY_LEFT_SHIFT])
		g_Camera.ProcessKeyboard(LEFT, deltaTime * speed);
	if (g_Keys[GLFW_KEY_D] && g_Keys[GLFW_KEY_LEFT_SHIFT])
		g_Camera.ProcessKeyboard(RIGHT, deltaTime * speed);

	if (g_Keys[GLFW_KEY_W])
		g_Camera.ProcessKeyboard(FORWARD, deltaTime);
	if (g_Keys[GLFW_KEY_S])
		g_Camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (g_Keys[GLFW_KEY_A])
		g_Camera.ProcessKeyboard(LEFT, deltaTime);
	if (g_Keys[GLFW_KEY_D])
		g_Camera.ProcessKeyboard(RIGHT, deltaTime);

	GLfloat velocity = 300.0f * deltaTime;
	if (g_Keys[GLFW_KEY_UP] && g_Keys[GLFW_KEY_LEFT_SHIFT])
		g_Camera.ProcessMouseMovement(0, velocity*speed);
	if (g_Keys[GLFW_KEY_DOWN] && g_Keys[GLFW_KEY_LEFT_SHIFT])
		g_Camera.ProcessMouseMovement(0, -velocity*speed);
	if (g_Keys[GLFW_KEY_LEFT] && g_Keys[GLFW_KEY_LEFT_SHIFT])
		g_Camera.ProcessMouseMovement(-velocity*speed, 0);
	if (g_Keys[GLFW_KEY_RIGHT] && g_Keys[GLFW_KEY_LEFT_SHIFT])
		g_Camera.ProcessMouseMovement(velocity*speed, 0);
	if (g_Keys[GLFW_KEY_UP])
		g_Camera.ProcessMouseMovement(0, velocity);
	if (g_Keys[GLFW_KEY_DOWN])
		g_Camera.ProcessMouseMovement(0, -velocity);
	if (g_Keys[GLFW_KEY_LEFT])
		g_Camera.ProcessMouseMovement(-velocity, 0);
	if (g_Keys[GLFW_KEY_RIGHT])
		g_Camera.ProcessMouseMovement(velocity, 0);
}
void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
/*
	float lastX = g_WindowWidth / 2.0f;
	float lastY = g_WindowHeight / 2.0f;
	bool firstMouse = true;
	//if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		//camera.ProcessMouseMovement(xoffset, yoffset);
	}*/
}
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	g_Camera.ProcessMouseScroll(yoffset);
}

void bindTexture(GLenum textureUnit, GLenum textureType, GLuint texture)
{
	glActiveTexture(textureUnit);
	glBindTexture(textureType, texture);
}

void renderImgui()
{
	// Rendering
	ImGui::Render();
	int display_w, display_h;
	//glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(g_Window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	//glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	//glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void setButtonColor(ImVec4 btnAfterClickColor)
{
	ImGuiStyle &style = ImGui::GetStyle();
	style.Colors[21] = btnAfterClickColor;
}
void revertButtonColor()
{
	ImGuiStyle &style = ImGui::GetStyle();
	style.Colors[21] = g_ButtonColor;
}

void releaseResource()
{
	if (g_LightModel)
	{
		delete g_LightModel;
		g_LightModel = NULL;
	}

	for (auto it = g_ModelMap.begin(); it != g_ModelMap.end(); it++)
	{
		Model *model = it->second;
		if (model)
		{
			delete model;
			model = NULL;
		}
	}
	g_ModelMap.clear();

	for (auto it = g_PBRMaterialMap.begin(); it != g_PBRMaterialMap.end(); it++)
	{
		PBRMaterial *material = it->second;
		if (material)
		{
			delete material;
			material = NULL;
		}
	}
	g_PBRMaterialMap.clear();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}