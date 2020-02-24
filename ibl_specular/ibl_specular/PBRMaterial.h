#pragma once
#include <string>
#include <glad/glad.h>
#include "Utils.h"
using namespace std;

class PBRMaterial
{
public:
	bool isInit = false;
	bool isFlip = false;

	string showName;//按钮显示名称
	bool isShow;// 该材质是否作用于特定模型上[材质栏是否显示],默认显示
	// PBR纹理开关
	bool g_MergeMR = 0;
	bool g_HaveEmissive = 0;
	bool g_HaveAO = 0;
	// 预览图
	string previewPath;
	TextureInfo previewTexInfo;

	// 规定顺序
	// normal,albedo,metallic,roughness,ao,emissive
	vector<string> pathVec;
	vector<GLint> texVec;

public:
	PBRMaterial() 
	{
		isShow = true;
	}
	~PBRMaterial()
	{

	}

	void load()
	{
		if (!isInit)
		{
			for (int i = 0; i < pathVec.size(); i++)
			{
				string path = pathVec[i];
				// normal,albedo,metallic,roughness必有
				// metallic,roughness可能在一张纹理中
				// ao,emissive可有可无

				GLuint id = 0;
				if (
					(i == 0) ||
					(i == 1) ||
					(i == 2) ||
					(i == 3 && !g_MergeMR) ||
					(i == 4 && g_HaveAO) ||
					(i == 5 && g_HaveEmissive)
					)
				{
					id = createTexture(path.c_str(), false, isFlip).id;
				}

				texVec.push_back(id);
			}

			isInit = true;
		}
	}
	void loadPreviewTexture()
	{
		previewTexInfo = createTexture(previewPath.c_str(), false, isFlip);
	};

private:

};