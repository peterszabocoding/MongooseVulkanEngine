#include "shader.h"

#include <iostream>
#include <fstream>

namespace Raytracing {

    Shader::Shader()
    {
    }

    Shader::~Shader()
    {
    }

    std::string Shader::ReadFile(const char* filepath)
    {
        std::string content;
		std::ifstream fileStream(filepath, std::ios::in);

		if (!fileStream.is_open())
		{
			printf("Failed to read %s! File does not exist.", filepath);
			return "";
		}

		std::string line;
		while (!fileStream.eof())
		{
			std::getline(fileStream, line);
			content.append(line + "\n");
		}

		fileStream.close();
		return content;
    }
}