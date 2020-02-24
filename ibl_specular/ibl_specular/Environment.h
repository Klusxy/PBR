#pragma once

#include <string>
#include <glad/glad.h>
#include "Utils.h"
using namespace std;

struct EnvironmentTextureInfo
{
	EnvironmentTextureInfo()
	{
		hdr = false;
		flip = false;
		texId = 0;
		width = 0;
		height = 0;
		channel = 0;
	}
	string path;
	bool hdr;
	bool flip;
	GLuint texId;
	GLuint cubemapId;
	int width;
	int height;
	int channel;
};

class Environment
{
public:
	string showName;// 按钮显示名字
	bool isLoadTex;// 是否已经加载纹理
	EnvironmentTextureInfo previewTexInfo;
	EnvironmentTextureInfo environmentTexInfo;
	EnvironmentTextureInfo diffuseTexInfo;
	EnvironmentTextureInfo specularTexInfo;

public:
	Environment()
	{
		isLoadTex = false;
	};
	~Environment()
	{

	};

	void load(Shader envir2cubemapShader, Shader prefilterShader)
	{
		if (!isLoadTex)
		{
			{
				TextureInfo info = createTexture(environmentTexInfo.path.c_str(), environmentTexInfo.hdr, environmentTexInfo.flip);
				environmentTexInfo.texId = info.id;
			}
			{
				TextureInfo info = createTexture(diffuseTexInfo.path.c_str(), diffuseTexInfo.hdr, diffuseTexInfo.flip);
				diffuseTexInfo.texId = info.id;
			}
			{
				TextureInfo info = createTexture(specularTexInfo.path.c_str(), specularTexInfo.hdr, specularTexInfo.flip);
				specularTexInfo.texId = info.id;
			}

			environmentTexInfo.cubemapId = equirectangular_2_cubemap(envir2cubemapShader, environmentTexInfo.texId, 512, 512);
			diffuseTexInfo.cubemapId = equirectangular_2_cubemap(envir2cubemapShader, diffuseTexInfo.texId, 32, 32);
			//specularTexInfo.cubemapId = equirectangular_2_cubemap(envir2cubemapShader, specularTexInfo.texId, 128, 128);
			specularTexInfo.cubemapId = prefilter(prefilterShader, environmentTexInfo.cubemapId, 128, 128);
			isLoadTex = true;
		}
	}

	void loadPreviewTexture()
	{
		TextureInfo info = createTexture(previewTexInfo.path.c_str(), previewTexInfo.hdr, previewTexInfo.flip);
		previewTexInfo.texId = info.id;
		previewTexInfo.width = info.width;
		previewTexInfo.height = info.height;
	}

private:

};