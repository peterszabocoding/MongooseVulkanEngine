#!/bin/bash

echo "Compiling shaders..."

mkdir -p ./shader/spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/gbuffer.vert -o ./shader/spv/gbuffer.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/gbuffer.frag -o ./shader/spv/gbuffer.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/skybox.vert -o ./shader/spv/skybox.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/skybox.frag -o ./shader/spv/skybox.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/equirectangularToCubemap.vert -o ./shader/spv/equirectangularToCubemap.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/equirectangularToCubemap.frag -o ./shader/spv/equirectangularToCubemap.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/shadow_map.vert -o ./shader/spv/shadow_map.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/empty.frag -o ./shader/spv/empty.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/quad.vert -o ./shader/spv/quad.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/quad.frag -o ./shader/spv/quad.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/brdf.vert -o ./shader/spv/brdf.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/brdf.frag -o ./shader/spv/brdf.frag.spv

echo "Shader compilation finished"