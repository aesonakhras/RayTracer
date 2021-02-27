#pragma once
#include <string.h>
#include <iostream>
#include <fstream>
#include <assert.h>

#define NOMINMAX
#include <Windows.h>


#include "CustomVec.h"

class ExternalFile {
public:
	static std::string Load(const std::string fileName);
	
	ExternalFile() {};
	~ExternalFile() {};

	void BMPInit(int width, int height);
	void BMPBuffer(int x, int y, CV::ColorBMP color);
	void BMPWrite(std::string name);

private:
	unsigned char* m_data;
	unsigned char* m_imageBegin;

	int m_width;
	int m_height;
	int m_fileSize;
	
};