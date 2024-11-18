#version 450

layout(location = 5) in vec2 inTexCoord;

// 材质参数
layout(binding = 1) uniform sampler2D diffuseMap;
layout (location = 0) out vec4 outColor;
void main() {
    // 采样纹理
    vec4 diffuseColor = texture(diffuseMap, inTexCoord);
    outColor = diffuseColor;
}