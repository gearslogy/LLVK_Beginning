#version 460 core
#include "math.glsl"

// -- IN --
layout(location=0) in vec3 P;
layout(location=1) in vec3 N;
layout(location=2) in vec3 T;
layout(location=3) in vec2 uv0;


// -- OUT --
layout(location = 0) out vec3 wN;
layout(location = 1) out vec3 wT;
layout(location = 2) out vec3 wB;
layout(location = 3) out vec3 wP;
layout(location = 4) out vec2 vel;
layout(location = 5) out vec2 uv;


layout(set=0 , binding =0 ) uniform UBO{
    mat4 proj;
    mat4 view;
    mat4 preProj;
    mat4 preView;
    vec4 cameraPosition;
}ubo;

layout(push_constant) uniform PushConstant {
    mat4 model;
    mat4 preModel;
} pcv;


void main(){
    vec4 worldPos = pcv.model * vec4(P,1.0);
    vec4 currentPos = ubo.proj * ubo.view * worldPos;
    gl_Position = currentPos;

    mat3 matN = normal_matrix(pcv.model);
    wN =  matN * normalize(N);
    wT = matN * normalize(T);
    wB = matN * normalize(cross(wN,wT));

    vec4 previousPos = ubo.preProj * ubo.preView * pcv.preModel * vec4(P, 1.0);

    // 转换到NDC空间
    vec2 currentNDC = currentPos.xy / currentPos.w;
    vec2 previousNDC = previousPos.xy / previousPos.w;

    // 计算速度向量（当前-上一帧）
    vel = (currentNDC - previousNDC) * 0.5; // 可以缩放
    uv = uv0;
    wP = worldPos.xyz;
}