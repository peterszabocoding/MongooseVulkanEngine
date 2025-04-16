#pragma once

#include "core.h"
#include <spdlog/spdlog.h>

namespace Raytracing
{
    class Log {
    public:
        static void Init();
        static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }

    private:
        static Ref<spdlog::logger> s_CoreLogger;
    };
}

#define LOG_TRACE(...) Raytracing::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...) Raytracing::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) Raytracing::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)	Raytracing::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_FATAL(...)	Raytracing::Log::GetCoreLogger()->fatal(__VA_ARGS__)
