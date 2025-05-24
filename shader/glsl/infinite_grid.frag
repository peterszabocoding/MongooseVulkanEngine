#version 450
#extension GL_ARB_shading_language_include : require

#include <common.glslh>

// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 out_camPos;
layout(location = 2) in vec3 worldPos;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) out vec4 FragColor;

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

layout(set = 1, binding = 0) uniform Transforms {
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
} transforms;

// ------------------------------------------------------------------
// CONSTANTS --------------------------------------------------------
// ------------------------------------------------------------------

const float GRID_MIN_PIXELS_BETWEEN_CELLS = 2.0;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec4 gridColorThick;
vec4 gridColorThin;

vec4 gridColor(vec2 uv, vec2 camPos) {
    vec2 dudv = vec2( length(vec2(dFdx(uv.x), dFdy(uv.x))),  length(vec2(dFdx(uv.y), dFdy(uv.y))) );

    float lodLevel = max(0.0, log10((length(dudv) * GRID_MIN_PIXELS_BETWEEN_CELLS) / push.gridCellSize) + 1.0);
    float lodFade = fract(lodLevel);

    // cell sizes for lod0, lod1 and lod2
    float lod0 = push.gridCellSize * pow(10.0, floor(lodLevel));
    float lod1 = lod0 * 10.0;
    float lod2 = lod1 * 10.0;

    // each anti-aliased line covers up to 4 pixels
    dudv *= 2.0;

    // set grid coordinates to the centers of anti-aliased lines for subsequent alpha calculations
    uv += dudv * 0.5;

    // calculate absolute distances to cell line centers for each lod and pick max X/Y to get coverage alpha value
    float lod0a = max2( vec2(1.0) - abs(satv(mod(uv, lod0) / dudv) * 2.0 - vec2(1.0)) );
    float lod1a = max2( vec2(1.0) - abs(satv(mod(uv, lod1) / dudv) * 2.0 - vec2(1.0)) );
    float lod2a = max2( vec2(1.0) - abs(satv(mod(uv, lod2) / dudv) * 2.0 - vec2(1.0)) );

    uv -= camPos;

    // blend between falloff colors to handle LOD transition
    vec4 c = lod2a > 0.0 ? gridColorThick : lod1a > 0.0 ? mix(gridColorThick, gridColorThin, lodFade) : gridColorThin;

    // calculate opacity falloff based on distance to grid extents
    float opacityFalloff = (1.0 - satf(length(uv) / push.gridSize));

    // blend between LOD level alphas and scale with opacity falloff
    c.a *= (lod2a > 0.0 ? lod2a : lod1a > 0.0 ? lod1a : (lod0a * (1.0-lodFade))) * opacityFalloff;

    return c;
}

void main() {
    gridColorThick = vec4(push.gridColorThick, 1.0);
    gridColorThin = vec4(push.gridColorThin, 1.0);

    FragColor = gridColor(uv, out_camPos);
}