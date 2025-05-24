#version 450

// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec3 inPosition;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) out vec2 uv;
layout(location = 1) out vec2 out_camPos;
layout(location = 2) out vec3 worldPos;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(push_constant) uniform Push {
    vec3 gridColorThin;
    vec3 gridColorThick;
    vec3 origin;
    float gridSize;
    float gridCellSize;
} push;

layout(set = 0, binding = 0) uniform Transforms {
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
} transforms;


void main() {
    vec3 position = inPosition * push.gridSize;
    position.x += transforms.cameraPosition.x;
    position.z += transforms.cameraPosition.z;
    position += push.origin.xyz;

    out_camPos = transforms.cameraPosition.xz;
    gl_Position = transforms.projection * transforms.view * vec4(position, 1.0);
    uv = position.xz;

    worldPos = position;
}