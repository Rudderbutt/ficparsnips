// ShaderLoader.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ShaderLoader.h"

ShaderLoader::File ShaderLoader::ReadShaderFile(std::string _filePath)
{
	File f;

	char res[_MAX_PATH];
	std::string fp(res, GetModuleFileNameA(NULL, res, _MAX_PATH));

	char drive[_MAX_DRIVE], dir[_MAX_DIR], fn[_MAX_FNAME], ext[_MAX_EXT];

	_splitpath_s(fp.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, fn, _MAX_FNAME, ext, _MAX_EXT);

	fp.clear();

	fp.append(drive);
	fp.append(dir);
	fp.append(_filePath);

	FILE * file = nullptr;
	fopen_s(&file, fp.c_str(), "rb");

	if (!file)
		return f;

	std::vector<byte> data;

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	data.resize(size);

	fread(&data[0], sizeof(uint8_t), size, file);
	fclose(file);

	f.stream = data;

	return f;
}
