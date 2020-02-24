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

	string showName;//��ť��ʾ����
	bool isShow;// �ò����Ƿ��������ض�ģ����[�������Ƿ���ʾ],Ĭ����ʾ
	// PBR������
	bool g_MergeMR = 0;
	bool g_HaveEmissive = 0;
	bool g_HaveAO = 0;
	// Ԥ��ͼ
	string previewPath;
	TextureInfo previewTexInfo;

	// �涨˳��
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
				// normal,albedo,metallic,roughness����
				// metallic,roughness������һ��������
				// ao,emissive���п���

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