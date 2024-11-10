#version 460 core
#include "math.glsl"

layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec3 T;
layout(location=4) in vec3 B;
layout(location=5) in vec2 uv0;
layout(location=6) in vec2 uv1;

// depth pass only rely on the uv
layout(location = 0) out vec2 out_uv0;


void main() {
    // 直接传递世界空间位置给几何着色器
    gl_Position = vec4(P, 1.0);
    // 传递纹理坐标给几何着色器
    out_uv0 = uv0;
}
