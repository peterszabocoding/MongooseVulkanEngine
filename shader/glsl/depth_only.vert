#version 450
#extension GL_EXT_scalar_block_layout : require

// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec3 inPosition;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(push_constant) uniform Push {
    mat4 projection;
    mat4 modelMatrix;
} push;

// ------------------------------------------------------------------

void main()
{
    gl_Position = push.projection * push.modelMatrix * vec4(inPosition, 1.0);
}