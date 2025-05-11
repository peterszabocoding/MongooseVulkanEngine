#!/bin/bash

echo "Compiling shaders..."

mkdir -p ./shader/spv

"$VULKAN_SDK"/bin/glslc ./shader/glsl/gbuffer.vert -o ./shader/spv/gbuffer.vert.spv
"$VULKAN_SDK"/bin/glslc ./shader/glsl/gbuffer.frag -o ./shader/spv/gbuffer.frag.spv

"$VULKAN_SDK"/bin/glslc ./shader/glsl/skybox.vert -o ./shader/spv/skybox.vert.spv
"$VULKAN_SDK"/bin/glslc ./shader/glsl/skybox.frag -o ./shader/spv/skybox.frag.spv

"$VULKAN_SDK"/bin/glslc ./shader/glsl/equirectangularToCubemap.vert -o ./shader/spv/equirectangularToCubemap.vert.spv
"$VULKAN_SDK"/bin/glslc ./shader/glsl/equirectangularToCubemap.frag -o ./shader/spv/equirectangularToCubemap.frag.spv

"$VULKAN_SDK"/bin/glslc ./shader/glsl/shadow_map.vert -o ./shader/spv/shadow_map.vert.spv
"$VULKAN_SDK"/bin/glslc ./shader/glsl/empty.frag -o ./shader/spv/empty.frag.spv

"$VULKAN_SDK"/bin/glslc ./shader/glsl/quad.vert -o ./shader/spv/quad.vert.spv
"$VULKAN_SDK"/bin/glslc ./shader/glsl/quad.frag -o ./shader/spv/quad.frag.spv


echo "Shader compilation finished"