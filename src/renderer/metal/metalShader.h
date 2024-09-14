#pragma once

#include "renderer/shader.h"

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

namespace Raytracing {

    class MetalShader: public Shader {

        public:
            MetalShader(MTL::Device* device, const char* shaderFile);
            virtual ~MetalShader();

            virtual void* CompileShader(const std::string& shaderCode) override;

            MTL::RenderPipelineState* GetRenderPipelineState() const { return _pPSO; }

        private:
            MTL::Device* _pDevice;
            MTL::RenderPipelineState* _pPSO;
    };

}