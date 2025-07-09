#include "util/log.h"

#include "spdlog/sinks/stdout_color_sinks-inl.h"

namespace Raytracing
{
    void Log::Init()
    {
        spdlog::set_pattern("%^[%T] %n: %v%$");
        s_CoreLogger = spdlog::stdout_color_mt("Renderer");
        s_CoreLogger->set_level(spdlog::level::trace);
    }
    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
}
