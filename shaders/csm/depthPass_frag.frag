#version 460 core
layout(location = 0) in vec2 uv0;

// 用于alpha测试的纹理
layout(set = 0, binding = 1) uniform sampler2D albedoTexture; // one set

void main() {
    // 采样纹理
    vec4 color = texture(albedoTexture, uv0);

    // Alpha测试
    if(color.a < 0.5) {  // 可以根据需要调整阈值
        discard;  // 丢弃透明或半透明片段
    }

    // 不需要输出颜色，深度值会自动写入
    // gl_FragDepth = gl_FragCoord.z;  // 可选，如果需要修改深度值
}