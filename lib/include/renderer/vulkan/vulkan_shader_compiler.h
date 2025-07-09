#pragma once

#include <shaderc/shaderc.hpp>

#include "util/core.h"

namespace MongooseVK
{

    struct CompilationInfo {
        std::string fileName;
        shaderc_shader_kind kind;
        std::vector<char> source{};
        shaderc::CompileOptions options{};
    };

    struct CompilationResult {
        bool success = false;
        std::vector<uint32_t> spirv;
        std::string errorMessage;
    };

    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
    public:
        virtual shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source,
                                                   size_t include_depth) override;

        virtual void ReleaseInclude(shaderc_include_result* data) override;
    };

    class VulkanShaderCompiler {
    public:
        VulkanShaderCompiler();
        ~VulkanShaderCompiler();

        void PreprocessShader(CompilationInfo& info);
        void CompileFileToAssembly(CompilationInfo& info);
        std::vector<uint32_t> CompileFile(CompilationInfo& info);

    private:
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
    };
}
