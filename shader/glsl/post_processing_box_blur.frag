// Credit:
// https://lettier.github.io/3d-game-shaders-for-beginners/blur.html

#version 450

// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 2) in vec2 TexCoords;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) out vec4 FragColor;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(set = 0, binding = 0) uniform sampler2D colorTexture;

layout(push_constant) uniform Push {
    float blurIntensity;
} push;

void main()
{
    vec2 parameters = vec2(push.blurIntensity, push.blurIntensity);
    vec2 texSize  = textureSize(colorTexture, 0).xy;

    int size = int(parameters.x);
    if (size <= 0) {
        FragColor = texture(colorTexture, TexCoords);
        return;
    }

    float separation = parameters.y;
    separation = max(separation, 0);

    for (int i = -size; i <= size; ++i) {
        for (int j = -size; j <= size; ++j) {
            FragColor += texture(colorTexture, (gl_FragCoord.xy + (vec2(i, j) * separation)) / texSize);
        }
    }

    FragColor /= pow(size * 2 + 1, 2);
}
