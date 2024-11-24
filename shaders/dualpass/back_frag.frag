#version 450
#include "common.glsl"
layout(location = 2) in vec3 N;
layout(location = 5) in vec2 inTexCoord;

// 材质参数
layout(binding = 1) uniform sampler2D diffuseMap;
layout (location = 0) out vec4 outColor;
float linearizeDepth(float depth) {
    float n = 0.1; // 近平面
    float f = 2500.0; // 远平面
    float z = depth * 2.0 - 1.0; // 转回 [-1,1] 范围
    return (2.0 * n * f) / (f + n - z * (f - n));
}
void main() {
    vec4 color = texture(diffuseMap, inTexCoord);
    // Alpha测试
    color = gammaCorrect(color,2.2);
    /*
    // 可选：添加深度基础的fade
    float depth = gl_FragCoord.z;
    float fade = 1.0 - depth; // 或其他深度衰减公式

    // 可选：柔化边缘
    float alpha = color.a * fade;
    outColor = vec4(color.rgb * alpha, alpha );
    */
    float depth = gl_FragCoord.z;
    float alpha = color.a;

    float top = dot (N, vec3(0,1,0));
    top= max(top, 0.2 );


    outColor = vec4(color.rgb  * alpha * depth * top, alpha );

}