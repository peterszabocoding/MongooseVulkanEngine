#!/bin/bash

mkdir -p ./shader/spv

/Users/peterszabo/VulkanSDK/1.3.290.0/macOS/bin/glslc ./shader/glsl/shader.vert -o ./shader/spv/vert.spv
/Users/peterszabo/VulkanSDK/1.3.290.0/macOS/bin/glslc ./shader/glsl/shader.frag -o ./shader/spv/frag.spv
