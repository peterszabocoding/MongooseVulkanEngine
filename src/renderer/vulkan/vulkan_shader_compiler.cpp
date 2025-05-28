#include "vulkan_shader_compiler.h"

#include <filesystem>
#include <sstream>

#include "util/core.h"
#include "util/filesystem.h"
#include "util/log.h"

namespace Raytracing
{
    shaderc_include_result* ShaderIncluder::GetInclude(const char* requested_source, shaderc_include_type type,
                                                       const char* requesting_source, size_t include_depth)
    {
        const auto includePath = std::string("shader\\glsl\\includes/").append(requested_source);
        const auto includeSourceRaw = FileSystem::ReadFile(includePath);

        const auto result = new shaderc_include_result(); {

            const uint32_t sourceLength = includeSourceRaw.size();
            char* includeSource = (char*) malloc(sourceLength);
            memcpy(includeSource, includeSourceRaw.data(), includeSourceRaw.size());

            const std::string requestedSourcePath = std::filesystem::current_path().append(requested_source).string();
            const uint32_t sourcePathLength = requestedSourcePath.size();
            char* sourcePath = (char*) malloc(sourcePathLength);
            memcpy(sourcePath, requestedSourcePath.c_str(), sourcePathLength);

            result->content = includeSource;
            result->content_length = sourceLength;
            result->source_name = sourcePath;
            result->source_name_length = sourcePathLength;
        }

        return result;
    }

    void ShaderIncluder::ReleaseInclude(shaderc_include_result* data)
    {
        delete data;
    }

    VulkanShaderCompiler::VulkanShaderCompiler() {}
    VulkanShaderCompiler::~VulkanShaderCompiler() {}

    void VulkanShaderCompiler::PreprocessShader(CompilationInfo& info)
    {
        LOG_INFO("Preprocessing shader: {0}", info.fileName);

        info.options.SetIncluder(std::make_unique<ShaderIncluder>());
        info.source = FileSystem::ReadFile(info.fileName);

        const auto result = compiler.PreprocessGlsl(info.source.data(),
                                                    info.source.size(),
                                                    info.kind,
                                                    info.fileName.c_str(),
                                                    info.options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
            ASSERT(false, "Failed to preprocess shader: " + result.GetErrorMessage());

        const char* src = result.cbegin();
        const size_t newSize = result.cend() - src;
        info.source.resize(newSize);
        memcpy(info.source.data(), src, newSize);

        LOG_INFO("-------------- Preprocessed GLSL source code -------------------");
        const std::string output(info.source.begin(), info.source.end());
        LOG_TRACE(output);
    }

    void VulkanShaderCompiler::CompileFileToAssembly(CompilationInfo& info)
    {
        LOG_INFO("Compile shader to assembly: {0}", info.fileName);

        info.source = FileSystem::ReadFile(info.fileName);

        const auto result = compiler.CompileGlslToSpvAssembly(info.source.data(),
                                                              info.source.size(),
                                                              info.kind,
                                                              info.fileName.c_str(),
                                                              info.options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
            ASSERT(false, "Failed to compile SPIR-V assembly: " + result.GetErrorMessage());

        const char* src = result.cbegin();
        const size_t newSize = result.cend() - src;
        info.source.resize(newSize);
        memcpy(info.source.data(), src, newSize);

        LOG_INFO("-------------- SPIR-V Assembly Code -------------------");
        const std::string output(info.source.begin(), info.source.end());
        LOG_TRACE(output);
    }

    std::vector<uint32_t> VulkanShaderCompiler::CompileFile(CompilationInfo& info)
    {
        LOG_INFO("Compile shader: {0}", info.fileName);

        info.source = FileSystem::ReadFile(info.fileName);
        info.options.SetIncluder(std::make_unique<ShaderIncluder>());

        //const auto result = compiler.AssembleToSpv(info.source.data(), info.source.size(), info.options);
        const auto preprocessResult = compiler.PreprocessGlsl(info.source.data(),
                                                              info.source.size(),
                                                              info.kind,
                                                              info.fileName.c_str(),
                                                              info.options);

        if (preprocessResult.GetCompilationStatus() != shaderc_compilation_status_success)
            ASSERT(false, "Failed to preprocess shader: " + preprocessResult.GetErrorMessage()); {
            const char* src = preprocessResult.cbegin();
            const size_t newSize = preprocessResult.cend() - src;
            info.source.resize(newSize);
            memcpy(info.source.data(), src, newSize);
        }

        const auto result = compiler.CompileGlslToSpv(info.source.data(), info.source.size(), info.kind, info.fileName.c_str(), "main", options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success)
            ASSERT(false, "Failed to assemble to SPIR-V: " + result.GetErrorMessage());

        std::vector<uint32_t> output; {
            const uint32_t* src = result.cbegin();
            const size_t wordCount = result.cend() - src;
            output.resize(wordCount);
            memcpy(output.data(), src, wordCount * sizeof(uint32_t));
        }

        LOG_INFO("-------------- SPIR-V Binary Code -------------------");
        std::stringstream converter;
        converter << output[0];
        LOG_TRACE("Magic number: {0}", converter.str());

        return output;
    }
}
