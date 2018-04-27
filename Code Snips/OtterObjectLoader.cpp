#include "stdafx.h"
#include "OtterObjectLoader.h"

OtterObjectLoader::OtterObjectLoader()
{
	loadComplete = false;
}

void OtterObjectLoader::LoadModelsAsync(std::string _filePath)
{

	FileThreadData ftd;
	strcpy_s(ftd.filePath, _filePath.c_str());
	ftd.fileMutex = &loadMutex;
	auto thread = std::thread(&OtterObjectLoader::FileLoadThreadEntry, this, ftd);

	thread.join();

	loadComplete = true;
}

void OtterObjectLoader::FileLoadThreadEntry(FileThreadData threadData)
{
	OtterModel model;

	std::vector<std::string> file;

	FILE* openFile;
	fopen_s(&openFile, threadData.filePath, "r");

	//char filename[_MAX_FNAME], drive[_MAX_DRIVE], dir[_MAX_DIR], ext[_MAX_EXT];
	//
	//_splitpath_s(threadData.filePath, drive, _MAX_DRIVE, dir, _MAX_DIR, filename, _MAX_FNAME, ext, _MAX_EXT);
	//
	//strcpy_s(model.modelName, _MAX_FNAME, filename);

	if (openFile == nullptr)
		return;

	std::vector<OtterPosition> vertices;
	std::vector<OtterUV> uv;
	std::vector<OtterPosition> norm;
	std::vector<unsigned short> vIndices;
	std::vector<unsigned short> uIndices;
	std::vector<unsigned short> nIndices;

	while (true)
	{
		char lineHeader[128];

		int result = fscanf_s(openFile, "%s", lineHeader, (unsigned int) sizeof(lineHeader));

		if (result == EOF)
			break;

		if (!strcmp(lineHeader, "v"))
		{
			float x, y, z;

			fscanf_s(openFile, "%f %f %f\n", &x, &y, &z);

			vertices.push_back(OtterPosition(x, y, z));
		}
		else if (!strcmp(lineHeader, "vt"))
		{
			float u, v;

			fscanf_s(openFile, "%f %f\n", &u, &v);

			uv.push_back(OtterUV(u, v));
		}
		else if (!strcmp(lineHeader, "vn"))
		{
			float nX, nY, nZ;

			fscanf_s(openFile, "%f %f %f\n", &nX, &nY, &nZ);

			norm.push_back(OtterPosition(nX, nY, nZ));
		}
		else if (!strcmp(lineHeader, "f"))
		{
			unsigned int x, y, z, uX, uY, uZ, nX, nY, nZ;

			result = fscanf_s(openFile, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &x, &uX, &nX, &y, &uY, &nY, &z, &uZ, &nZ);

			if (result != 9)
				return;

			vIndices.push_back(x - 1);
			vIndices.push_back(y - 1);
			vIndices.push_back(z - 1);

			uIndices.push_back(uX - 1);
			uIndices.push_back(uY - 1);
			uIndices.push_back(uZ - 1);

			nIndices.push_back(nX - 1);
			nIndices.push_back(nY - 1);
			nIndices.push_back(nZ - 1);
		}
	}

	for (int i = 0; i < vIndices.size(); i++)
	{
		//VertexPositionColor vpc = { XMFLOAT3(vertices[i].x, vertices[i].y, vertices[i].z), XMFLOAT3(norm[i].x, norm[i].y, norm[i].z) };

		OtterVertex vpun{ OtterPosition(vertices[vIndices[i]].x, vertices[vIndices[i]].y, vertices[vIndices[i]].z), OtterPosition(norm[nIndices[i]].x, norm[nIndices[i]].y, norm[nIndices[i]].z), OtterUV(uv[uIndices[i]].u, (1 - uv[uIndices[i]].v)) };
		model.vIndices.push_back(i);
		//model.modelVerticesVPC.push_back(vpc);
		model.vertices.push_back(vpun);
	}

	threadData.fileMutex->lock();

	loadedModels.push_back(model);

	threadData.fileMutex->unlock();
}

OtterObjectLoader::OtterModel OtterObjectLoader::GetModelInformation()
{
	return loadedModels[0];
}

bool OtterObjectLoader::IsLoadComplete()
{
	return loadComplete;
}
