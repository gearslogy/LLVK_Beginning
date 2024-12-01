#version 460 core
layout(location = 0) in vec2 uv0;

// 用于alpha测试的纹理
layout(set = 0, binding = 2) uniform sampler2D albedoTexture; // one set

void main() {
    vec4 color = texture(albedoTexture, uv0);

    if(color.a < 0.5) {  // 可以根据需要调整阈值
        discard;  // 丢弃透明或半透明片段
    }
}