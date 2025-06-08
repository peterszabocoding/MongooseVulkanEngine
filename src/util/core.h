#pragma once
#include <memory>
#include <string>
#include "log.h"

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

#define ASSERT_STRINGIFY_MACRO(x) #x

#ifdef ENABLE_ASSERTS
#ifdef PLATFORM_MACOS
#define ASSERT(x, ...)
#else
#define ASSERT(x, ...) { if(!(x)) { LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#endif
#else
#define ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define CAST_REF(T, x) std::static_pointer_cast<T>(x)

template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T>
using Ref = std::shared_ptr<T>;

template<class T>
using Scope = std::unique_ptr<T>;

template<class T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename E>
constexpr auto EnumValue(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}
