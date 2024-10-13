#pragma once
#include <string>

struct Version
{
	int major;
	int minor;
	int patch;

	std::string GetVersionName()
	{
		return std::to_string(major) + '.' + std::to_string(minor) + "." + std::to_string(patch);
	}
};

const std::string APPLICATION_NAME = "RaytracingInOneWeekend";
constexpr Version APPLICATION_VERSION = {0, 1, 0};
