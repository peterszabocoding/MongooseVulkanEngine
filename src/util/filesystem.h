#pragma once

#include <fstream>
#include <stdexcept>
#include <vector>

namespace Raytracing
{
	namespace FileSystem
	{
		static std::vector<char> ReadFile(const std::string& filepath)
		{
			std::ifstream file(filepath, std::ios::ate | std::ios::binary);

			if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filepath);

			const size_t file_size = file.tellg();
			std::vector<char> buffer(file_size);

			file.seekg(0);
			file.read(buffer.data(), file_size);

			file.close();

			return buffer;
		}
	}
}
