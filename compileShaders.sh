#!/bin/bash

echo "Compiling shaders..."

mkdir -p ./shader/spv

$VULKAN_SDK/bin/glslc ./shader/glsl/gbuffer.vert -o ./shader/spv/gbuffer.vert.spv
$VULKAN_SDK/bin/glslc ./shader/glsl/gbuffer.frag -o ./shader/spv/gbuffer.frag.spv

$VULKAN_SDK/bin/glslc ./shader/glsl/screen.vert -o ./shader/spv/screen.vert.spv
$VULKAN_SDK/bin/glslc ./shader/glsl/screen.frag -o ./shader/spv/screen.frag.spv

echo "Shader compilation finished"