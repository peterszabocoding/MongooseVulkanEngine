#!/bin/bash

echo "Compiling shaders..."

mkdir -p ./shader/spv

$VULKAN_SDK/bin/glslc ./shader/glsl/shader.vert -o ./shader/spv/vert.spv
$VULKAN_SDK/bin/glslc ./shader/glsl/shader.frag -o ./shader/spv/frag.spv

echo "Shader compilation finished"