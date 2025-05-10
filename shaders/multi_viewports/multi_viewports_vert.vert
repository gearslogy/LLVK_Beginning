#version 460 core

// 注意这里不能携程接口块
layout(location=0) in vec3 P;
layout(location=1) in vec3 N;
layout(location=2) in vec3 T;
layout(location=3) in vec2 uv0;


layout(location=0) out OUTPUT{
    vec3 N;
    vec3 T;
    vec2 uv0;
}vs_out;

void main(){
    gl_Position = vec4(P, 1.0);

    vs_out.N = N;
    vs_out.T = T;
    vs_out.uv0 = uv0;
}