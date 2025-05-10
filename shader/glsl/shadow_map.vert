#version 450
#extension GL_EXT_scalar_block_layout : require

// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec3 inPosition;

// ------------------------------------------------------------------
// STRUCTS ----------------------------------------------------------
// ------------------------------------------------------------------



// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 modelMatrix;
} push;

layout(std430, set = 0, binding = 0) uniform Lights {
    mat4 lightView;
    mat4 lightProjection;
    vec3 direction;
    float ambientIntensity;
    vec4 color;
    float intensity;
    float bias;
} lights;

// ------------------------------------------------------------------

void main()
{
    gl_Position = lights.lightProjection * lights.lightView * push.modelMatrix * vec4(inPosition, 1.0);
}