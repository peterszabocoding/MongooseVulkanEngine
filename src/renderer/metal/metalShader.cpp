#include "metalShader.h"

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include <iostream>

namespace Raytracing {

    MetalShader::MetalShader(MTL::Device *device, const char* shaderFile): _pDevice(device) 
    {
        CompileShader(shaderFile);
    }

    MetalShader::~MetalShader()
    {
        _pPSO->release();
    }

    void* MetalShader::CompileShader(const std::string& shaderSourceFile)
    {
        std::string shaderCode = ReadFile(shaderSourceFile.c_str());
        
        NS::Error* pError = nullptr;
        MTL::Library* pLibrary = _pDevice->newLibrary( 
            NS::String::string(shaderCode.c_str(), NS::StringEncoding::UTF8StringEncoding), 
            nullptr, 
            &pError
        );

        if ( !pLibrary )
        {
            if(pError)
            {
                std::clog << pError->localizedDescription()->utf8String() << std::endl;
                std::clog << pError->localizedFailureReason()->utf8String() << std::endl;
            }
            assert( false );
        }


        MTL::Function* pVertexFn = pLibrary->newFunction( NS::String::string("vertexMain", NS::StringEncoding::UTF8StringEncoding) );
        MTL::Function* pFragFn = pLibrary->newFunction( NS::String::string("fragmentMain", NS::StringEncoding::UTF8StringEncoding) );

        MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
        pDesc->setVertexFunction( pVertexFn );
        pDesc->setFragmentFunction( pFragFn );
        pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );

        _pPSO = _pDevice->newRenderPipelineState( pDesc, &pError );
        if ( !_pPSO )
        {
            if(pError)
            {
                std::clog << pError->localizedDescription()->utf8String() << std::endl;
                std::clog << pError->localizedFailureReason()->utf8String() << std::endl;
            }
            assert( false );
        }

        pVertexFn->release();
        pFragFn->release();
        pDesc->release();
        pLibrary->release();

        return nullptr;
    }

}
