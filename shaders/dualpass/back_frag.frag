#version 450
#include "common.glsl"
layout(location = 5) in vec2 inTexCoord;

// 材质参数
layout(binding = 1) uniform sampler2D diffuseMap;
layout (location = 0) out vec4 outColor;
void main() {
    vec4 color = texture(diffuseMap, inTexCoord);
    // Alpha测试
    //color = gammaCorrect(color,2.2);
    outColor = vec4(color.rgb , color.a);  // 保持原始 alpha
}