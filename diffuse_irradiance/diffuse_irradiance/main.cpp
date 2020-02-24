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

GLFWwindow *g_Window = 0;

// const
const int g_WindowWidth = 960;
const int g_WindowHeight = 600;
// light
glm::vec3 g_LightPosition(0.0f, 0.0f, 3.0f);
float g_LightColor[3] = { 300,300,300 };
// PBR
float g_Metallic = 0.5f;
float g_Smooth = 0.5f;
float g_Albedo[3] = {0.5,0,0};
//ImVec4 g_Albedo = ImColor(255, 255, 255, 255); // 表面颜色
// PBR texutre
GLuint g_AlbedoTexture = 0;
GLuint g_NormalTexture = 0;
GLuint g_MetallicTexture = 0;
GLuint g_RoughnessTexture = 0;
GLuint g_AOTexture = 0;
GLuint g_HDRTexture = 0;
// Sky box
GLuint g_CubemapTextureSkyBox = 0;
GLuint g_CubemapTextureConvert = 0;
// PreCompute
GLuint g_CubemapTextureIrradiance = 0;
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

void hdr2Cubemap(Shader shader, GLuint &destCubeTexture, GLuint hdrTexture);
void preComputeIrradiane(Shader convertShader, GLuint &destCubeTexture, GLuint hdrCubeTexture);

GLuint createTexture(const char* filename, bool hdr = false);
GLuint createCubemapTexture(vector<std::string> faces);

void renderCube();
void renderImgui();

void releaseResource();

int main()
{
	if (!initWindow())
		return -1;

	if (!initGLAD())
		return -1;

	if (!initImgui())
		return -1;

	Model sphere("../../3dModel/Sphere.obj");

	Shader pbrShader("./Model.vs", "./Model.fs");
	Shader lightShader("./Quad.vs", "./Quad.fs");
	Shader skyBoxShader("./SkyBox.vs", "./SkyBox.fs");
	Shader hdr2cubemapShader("./hdr2cubemap.vs", "./hdr2cubemap.fs");
	Shader preComputeIrradianceShader("./preComputeIrradiance.vs", "./preComputeIrradiance.fs");

	// Sky Box
	vector<std::string> faces
	{
		"../../resource/skybox/right.jpg",
		"../../resource/skybox/left.jpg",
		"../../resource/skybox/top.jpg",
		"../../resource/skybox/bottom.jpg",
		"../../resource/skybox/front.jpg",
		"../../resource/skybox/back.jpg"
	};
	g_CubemapTextureSkyBox = createCubemapTexture(faces);
	// gold grass plastic rusted_iron wall
	//g_AlbedoTexture = createTexture("../../resource/pbr/rusted_iron/albedo.png");
	//g_NormalTexture = createTexture("../../resource/pbr/rusted_iron/normal.png");
	//g_MetallicTexture = createTexture("../../resource/pbr/rusted_iron/metallic.png");
	//g_RoughnessTexture = createTexture("../../resource/pbr/rusted_iron/roughness.png");
	//g_AOTexture = createTexture("../../resource/pbr/rusted_iron/ao.png");
	g_HDRTexture = createTexture("../../resource/hdr/newport_loft.hdr", true);

	hdr2Cubemap(hdr2cubemapShader, g_CubemapTextureConvert, g_HDRTexture);
	preComputeIrradiane(preComputeIrradianceShader, g_CubemapTextureIrradiance, g_CubemapTextureConvert);
	
	while (!glfwWindowShouldClose(g_Window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//event
		glfwPollEvents();
		processInput(g_Window);

		// imgui
		{
			char window_title[50];
			sprintf_s(window_title, "PBR: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			glfwSetWindowTitle(g_Window, window_title);

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::Begin("Param");
			ImGui::SliderFloat("metallic", &g_Metallic, 0.0f, 1.0f, "%.3f");            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::SliderFloat("smooth", &g_Smooth, 0.0f, 1.0f, "%.3f");
			//ImGui::SliderFloat("albedo", &g_Albedo, 0.0f, 1.0f, "%.3f");
			//ImGui::ColorEdit3("albedo(Surface Color)", (float*)&g_Albedo);
			ImGui::InputFloat3(u8"light color", g_LightColor);
			ImGui::InputFloat3(u8"albedo(Surface Color)", g_Albedo);

			//static bool showDemo = true;
			//ImGui::ShowDemoWindow(&showDemo);

			ImGui::End();
		}

		glViewport(0, 0, g_WindowWidth, g_WindowHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_MULTISAMPLE);
		// 渲染PBR模型
		{
			pbrShader.use();
			glm::mat4 model;
			//model = glm::translate(model, glm::vec3(0, 0, -3.0f));
			//model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::vec3(0, 1, 0));
			//model = glm::scale(model, glm::vec3(3.0f));
			
			glm::mat4 view = g_Camera.GetViewMatrix();
			glm::mat4 projection = glm::perspective(glm::radians(g_Camera.Zoom), (GLfloat)g_WindowWidth / (GLfloat)g_WindowHeight, 0.1f, 1000.0f);
			pbrShader.setMat4("model", model);
			pbrShader.setMat4("view", view);
			pbrShader.setMat4("projection", projection);

			pbrShader.setVec3("lightPosition", g_LightPosition);
			pbrShader.setVec3("lightColor", glm::vec3(g_LightColor[0], g_LightColor[1], g_LightColor[2]));
			pbrShader.setVec3("cameraPosition", g_Camera.Position);

			pbrShader.setFloat("ao", 1.0f);
			pbrShader.setFloat("roughness", 1.0f - g_Smooth);
			pbrShader.setFloat("metallic", g_Metallic);
			pbrShader.setVec3("albedo", glm::vec3(g_Albedo[0], g_Albedo[1], g_Albedo[2]));

			pbrShader.setInt("albedoTexture", 0);
			pbrShader.setInt("metallicTexture", 1);
			pbrShader.setInt("roughnessTexture", 2);
			pbrShader.setInt("aoTexture", 3);
			pbrShader.setInt("normalTexture", 4);
			pbrShader.setInt("irradianceMap", 5);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, g_AlbedoTexture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, g_MetallicTexture);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, g_RoughnessTexture);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, g_AOTexture);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, g_NormalTexture);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubemapTextureIrradiance);

			sphere.Draw(pbrShader);
		}

		// 渲染灯
		{
			lightShader.use();
			glm::mat4 model;
			model = glm::translate(model, glm::vec3(0, 0, 3.0f));
			model = glm::scale(model, glm::vec3(0.05));
			glm::mat4 view = g_Camera.GetViewMatrix();
			glm::mat4 projection = glm::perspective(glm::radians(g_Camera.Zoom), (GLfloat)g_WindowWidth / (GLfloat)g_WindowHeight, 0.1f, 1000.0f);
			lightShader.setMat4("model", model);
			lightShader.setMat4("view", view);
			lightShader.setMat4("projection", projection);

			sphere.Draw(lightShader);
		}

		// 渲染天空盒
		{
			// draw skybox as last
			glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
			skyBoxShader.use();
			glm::mat4 view = glm::mat4(glm::mat3(g_Camera.GetViewMatrix())); // remove translation from the view matrix
			skyBoxShader.setMat4("view", view);
			glm::mat4 projection = glm::perspective(glm::radians(g_Camera.Zoom), (GLfloat)g_WindowWidth / (GLfloat)g_WindowHeight, 0.1f, 1000.0f);
			skyBoxShader.setMat4("projection", projection);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, g_CubemapTextureConvert);
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

std::string getExeDir()
{
	char exeFullPath[MAX_PATH];
	GetModuleFileNameA(nullptr, exeFullPath, MAX_PATH);
	std::string rootDir = exeFullPath;
	rootDir = rootDir.substr(0, rootDir.find_last_of('\\', rootDir.npos));
	return rootDir;
}
GLuint fbo = 0;
void hdr2Cubemap(Shader convertShader, GLuint &destCubeTexture, GLuint hdrTexture)
{
	if (fbo == 0)
	{
		glGenFramebuffers(1, &fbo);
	}

	if (destCubeTexture == 0)
	{
		glGenTextures(1, &destCubeTexture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, destCubeTexture);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	convertShader.use();

	convertShader.setInt("equirectangularMap", 0);
	convertShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTexture);

	glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for (unsigned int i = 0; i < 6; ++i)
	{
		convertShader.setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, destCubeTexture, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void preComputeIrradiane(Shader convertShader, GLuint &destCubeTexture, GLuint hdrCubeTexture)
{
	if (fbo == 0)
	{
		glGenFramebuffers(1, &fbo);
	}

	if (destCubeTexture == 0)
	{
		glGenTextures(1, &destCubeTexture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, destCubeTexture);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	convertShader.use();

	convertShader.setInt("environmentMap", 0);
	convertShader.setMat4("projection", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, hdrCubeTexture);

	glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for (unsigned int i = 0; i < 6; ++i)
	{
		convertShader.setMat4("view", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, destCubeTexture, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	g_Window = glfwCreateWindow(g_WindowWidth, g_WindowHeight, "SoftRenderer", NULL, NULL);
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

GLuint createTexture(const char* filename, bool hdr)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// 加载并生成纹理
	stbi_set_flip_vertically_on_load(true);
	int w = 0, h = 0, ch = 0;
	//unsigned char *data = stbi_load("./600x800.jpg", &w, &h, &ch, 0);
	// hdr格式的数据是float, unsigned char精度不够。图片偏亮
	unsigned char *data = 0;
	float *data_hdr = 0;
	if (hdr)
		data_hdr = stbi_loadf(filename, &w, &h, &ch, 0);
	else
		data = stbi_load(filename, &w, &h, &ch, 0);
	if (data || data_hdr)
	{
		GLint format = 0;
		if (ch == 4)
			format = GL_RGBA;
		if (ch == 3)
			format = GL_RGB;
		if (ch == 1)
			format = GL_RED;

		if (hdr)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, format, GL_FLOAT, data_hdr);
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}
GLuint createCubemapTexture(vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
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

void releaseResource()
{
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}