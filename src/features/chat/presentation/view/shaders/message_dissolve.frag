#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(binding = 0, std140) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float progress;
    vec2 itemSize;
};
layout(binding = 1) uniform sampler2D source;

float randomValue(vec2 cell)
{
    return fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    vec2 size = max(itemSize, vec2(1.0));
    vec2 pixel = qt_TexCoord0 * size;
    // Fine grains detach at different times and drift upward as they dissolve.
    vec2 cell = floor(pixel / 2.0);
    float seed = randomValue(cell);
    float age = clamp((progress - seed * 0.32) / 0.68, 0.0, 1.0);
    vec2 drift = vec2(10.0 + seed * 24.0, -12.0 - seed * 28.0) * age * age;
    vec2 uv = (pixel - drift) / size;
    float inside = step(0.0, uv.x) * step(uv.x, 1.0)
                 * step(0.0, uv.y) * step(uv.y, 1.0);
    float grain = randomValue(cell + vec2(39.7, 8.2));
    float coverage = 1.0 - smoothstep(grain * 0.7, grain * 0.7 + 0.3, age);
    fragColor = texture(source, uv) * coverage * inside * qt_Opacity;
}
