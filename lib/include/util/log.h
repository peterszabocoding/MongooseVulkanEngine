#pragma once

#include <spdlog/spdlog.h>

namespace MongooseVK
{
    class Log {
    public:
        static void Init();
        static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
    };
}

#define LOG_TRACE(...) MongooseVK::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...) MongooseVK::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) MongooseVK::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)	MongooseVK::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_FATAL(...)	MongooseVK::Log::GetCoreLogger()->fatal(__VA_ARGS__)
