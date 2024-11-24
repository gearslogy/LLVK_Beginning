#version 450
#include "common.glsl"
layout(location = 2) in vec3 N;
layout(location = 5) in vec2 inTexCoord;

// 材质参数
layout(binding = 1) uniform sampler2D diffuseMap;
layout (location = 0) out vec4 outColor;
void main() {
    // 采样纹理
    vec4 color = texture(diffuseMap, inTexCoord);
    // Alpha测试
    color = gammaCorrect(color,2.2);
    if (color.a < 0.9)
        discard;

    outColor = vec4(vec3(0), 0);  // alpha 设为 1
}