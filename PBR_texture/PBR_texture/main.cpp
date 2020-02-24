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
GLuint g_QuadVao = 0;
GLuint g_QuadVbo = 0;
//GLuint g_SoftRendererFrameBufferTex = 0;

// const
const int g_WindowWidth = 960;
const int g_WindowHeight = 600;
// light
glm::vec3 g_LightPosition(0.0f, 0.0f, 0.0f);
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
// camera
Camera  g_Camera(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90, 0);
bool g_Keys[1024];
bool spacePress = false;
bool g_Rotate = true;
float rotateHistory = 0.0f;
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

void createQuad();
GLuint createTexture(const char* filename);
//void createSoftRendererTexture();

void drawQuad();
void drawImgui();

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

	// gold grass plastic rusted_iron wall
	g_AlbedoTexture = createTexture("../../resource/pbr/rusted_iron/albedo.png");
	g_NormalTexture = createTexture("../../resource/pbr/rusted_iron/normal.png");
	g_MetallicTexture = createTexture("../../resource/pbr/rusted_iron/metallic.png");
	g_RoughnessTexture = createTexture("../../resource/pbr/rusted_iron/roughness.png");
	g_AOTexture = createTexture("../../resource/pbr/wall/ao.png");
	//createQuad();
	//createSoftRendererTexture();

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

/*
			ImGui::Begin("Param");
			ImGui::SliderFloat("metallic", &g_Metallic, 0.0f, 1.0f, "%.3f");            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::SliderFloat("smooth", &g_Smooth, 0.0f, 1.0f, "%.3f");
			//ImGui::SliderFloat("albedo", &g_Albedo, 0.0f, 1.0f, "%.3f");
			//ImGui::ColorEdit3("albedo(Surface Color)", (float*)&g_Albedo);
			ImGui::InputFloat3(u8"light color", g_LightColor);
			ImGui::InputFloat3(u8"albedo(Surface Color)", g_Albedo);

			//static bool showDemo = true;
			//ImGui::ShowDemoWindow(&showDemo);

			ImGui::End();*/
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		// 渲染PBR模型
		{
			pbrShader.use();
			glm::mat4 model;
			model = glm::translate(model, glm::vec3(0, 0, -10.0f));
			model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::vec3(0, 1, 0));
			//if (g_Rotate)
			//{
			//	model = glm::rotate(model, glm::radians(rotateHistory), glm::vec3(0, 1, 0));
			//	rotateHistory = (float)glfwGetTime() * -10.0f;
			//}
			model = glm::scale(model, glm::vec3(3.0f));
			
			glm::mat4 view = g_Camera.GetViewMatrix();
			glm::mat4 projection = glm::perspective(glm::radians(g_Camera.Zoom), (GLfloat)g_WindowWidth / (GLfloat)g_WindowHeight, 0.1f, 1000.0f);
			pbrShader.setMat4("model", model);
			pbrShader.setMat4("view", view);
			pbrShader.setMat4("projection", projection);

			pbrShader.setVec3("lightPosition", g_LightPosition);
			pbrShader.setVec3("lightColor", glm::vec3(g_LightColor[0], g_LightColor[1], g_LightColor[2]));
			pbrShader.setVec3("cameraPosition", g_Camera.Position);

/*
			pbrShader.setFloat("ao", 1.0f);
			pbrShader.setFloat("roughness", 1.0f - g_Smooth);
			pbrShader.setFloat("metallic", g_Metallic);
			pbrShader.setVec3("albedo", glm::vec3(g_Albedo[0], g_Albedo[1], g_Albedo[2]));*/

			pbrShader.setInt("albedoTexture", 0);
			pbrShader.setInt("metallicTexture", 1);
			pbrShader.setInt("roughnessTexture", 2);
			pbrShader.setInt("aoTexture", 3);
			pbrShader.setInt("normalTexture", 4);

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

			sphere.Draw(pbrShader);
		}

		// 渲染灯
		{
			lightShader.use();
			glm::mat4 objectModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.05));
			glm::mat4 view = g_Camera.GetViewMatrix();
			glm::mat4 projection = glm::perspective(glm::radians(g_Camera.Zoom), (GLfloat)g_WindowWidth / (GLfloat)g_WindowHeight, 0.1f, 1000.0f);
			lightShader.setMat4("model", objectModel);
			lightShader.setMat4("view", view);
			lightShader.setMat4("projection", projection);

			sphere.Draw(lightShader);
		}

		//render
		//draw_scene();

		//imgui
		drawImgui();

		glfwSwapBuffers(g_Window);
	}

	releaseResource();

	//release glfw
	glfwTerminate();

	return 0;
}

bool initGLAD()
{
	//init glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		return false;

	return true;
}

std::string getExeDir()
{
	char exeFullPath[MAX_PATH];
	GetModuleFileNameA(nullptr, exeFullPath, MAX_PATH);
	std::string rootDir = exeFullPath;
	rootDir = rootDir.substr(0, rootDir.find_last_of('\\', rootDir.npos));
	return rootDir;
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

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		spacePress = true;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
	{
		if (spacePress)
		{
			g_Rotate = !g_Rotate;
			printf("rotate: %d\n", g_Rotate);
			spacePress = false;
		}
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

GLuint createTexture(const char* filename)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// 加载并生成纹理
	stbi_set_flip_vertically_on_load(true);
	int w, h, ch;
	//unsigned char *data = stbi_load("./600x800.jpg", &w, &h, &ch, 0);
	unsigned char *data = stbi_load(filename, &w, &h, &ch, 0);
	if (data)
	{
		GLint format = 0;
		if (ch == 4)
			format = GL_RGBA;
		if (ch == 3)
			format = GL_RGB;
		if (ch == 1)
			format = GL_RED;
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

void createQuad()
{
	Shader shader("./Quad.vs", "./Quad.fs");
	shader.use();

	glViewport(0, 0, g_WindowWidth, g_WindowHeight);

	glm::mat4 model;
	shader.setMat4("model", model);
	glm::mat4 view;
	shader.setMat4("view", view);
	glm::mat4 projection;
	shader.setMat4("projection", projection);

	glGenVertexArrays(1, &g_QuadVao);
	glBindVertexArray(g_QuadVao);

	glGenBuffers(1, &g_QuadVbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_QuadVbo);
	GLfloat vertices[] =
	{
		-1.0f, -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f,  -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void*)(sizeof(GLfloat) * 2));
	glEnableVertexAttribArray(1);
}

/*
void createSoftRendererTexture()
{
	glGenTextures(1, &g_SoftRendererFrameBufferTex);
	glBindTexture(GL_TEXTURE_2D, g_SoftRendererFrameBufferTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}*/

void drawQuad()
{
	// 将软渲染结果当作纹理渲染到窗口上
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		//glBindTexture(GL_TEXTURE_2D, g_SoftRendererFrameBufferTex);
		glBindVertexArray(g_QuadVao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

void drawImgui()
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
	if (glIsBuffer(g_QuadVbo))
	{
		glDeleteBuffers(1, &g_QuadVbo);
		g_QuadVbo = 0;
	}

	if (glIsVertexArray(g_QuadVao))
	{
		glDeleteVertexArrays(1, &g_QuadVao);
		g_QuadVao = 0;
	}

	//if (glIsTexture(g_SoftRendererFrameBufferTex))
	//{
	//	glDeleteTextures(1, &g_SoftRendererFrameBufferTex);
	//	g_SoftRendererFrameBufferTex = 0;
	//}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}