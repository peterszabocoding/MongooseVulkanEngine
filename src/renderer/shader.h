#pragma once

#include <string>

namespace Raytracing {

    class Shader {

        public:
            Shader();
            virtual ~Shader();

            virtual void* CompileShader(const std::string& shaderCode) = 0;

        protected:
            std::string ReadFile(const char* filepath);

    };

}
