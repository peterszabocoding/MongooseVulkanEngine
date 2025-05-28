#!/bin/bash

echo "Compiling shaders..."

mkdir -p ./shader/spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/base-pass.vert -o ./shader/spv/base-pass.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/lighting-pass.frag -o ./shader/spv/lighting-pass.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/gbuffer.vert -o ./shader/spv/gbuffer.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/gbuffer.frag -o ./shader/spv/gbuffer.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/skybox.vert -o ./shader/spv/skybox.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/skybox.frag -o ./shader/spv/skybox.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/equirectangularToCubemap.vert -o ./shader/spv/equirectangularToCubemap.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/equirectangularToCubemap.frag -o ./shader/spv/equirectangularToCubemap.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/quad.vert -o ./shader/spv/quad.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/quad.frag -o ./shader/spv/quad.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/brdf.vert -o ./shader/spv/brdf.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/brdf.frag -o ./shader/spv/brdf.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/cubemap.vert -o ./shader/spv/cubemap.vert.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/depth_only.vert -o ./shader/spv/depth_only.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/empty.frag -o ./shader/spv/empty.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/irradiance_convolution.frag -o ./shader/spv/irradiance_convolution.frag.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/prefilter.frag -o ./shader/spv/prefilter.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/post_processing_ssao.frag -o ./shader/spv/post_processing_ssao.frag.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/post_processing_box_blur.frag -o ./shader/spv/post_processing_box_blur.frag.spv

"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/infinite_grid.vert -o ./shader/spv/infinite_grid.vert.spv
"$VULKAN_SDK"/bin/glslc -I ./shader/glsl/includes ./shader/glsl/infinite_grid.frag -o ./shader/spv/infinite_grid.frag.spv

echo "Shader compilation finished"