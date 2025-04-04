#version 460 core
#include "common.glsl"
// IN
layout(location = 0) in vec3 wN;
layout(location = 1) in vec3 wT;
layout(location = 2) in vec3 wB;
layout(location = 3) in vec3 wP;
layout(location = 4) in vec2 vel;
layout(location = 5) in vec2 uv;


layout(set=0, binding = 1) uniform sampler2D AlbedoTexSampler;
layout(set=0, binding = 2) uniform sampler2D NRMTexSampler;

// OUT : attachments
layout(location = 0) out vec4 rt_swapchain;
layout(location = 1) out vec4 rt_albedo;
layout(location = 2) out vec4 rt_NRM;
layout(location = 3) out vec4 rt_V;
layout(location = 4) out vec4 rt_P;

vec3 reconstructNormal(vec2 NXY){
    vec2 fenc = NXY * 2.0 - 1.0;    // 将 [0,1] 范围的值重映射到 [-1,1]
    float z = sqrt(1.0 - dot(fenc, fenc));    // 重建 Z 分量
    return normalize(vec3(fenc.x, fenc.y, z));    // 构建完整的法线向量
}

void main(){
    vec4 base = texture(AlbedoTexSampler, uv);
    //base = gammaCorrect(base,2.2);
    vec4 nrm = texture(NRMTexSampler, uv);
    nrm = gammaCorrect(nrm,2.2);

    vec3 normalmap = reconstructNormal(nrm.xy);
    mat3 TBN = mat3(wT, wB, wN);
    vec3 transformN = normalize(TBN * normalmap);

    // attachments out
    rt_albedo = base;          // 0
    rt_swapchain = vec4(0,0,0,0);    // 1
    rt_NRM = vec4(transformN , nrm.b) ;             // 2 [N.x,  N.y,  N.z,  roughness]
    rt_V= vec4(0);             // 3
    rt_V.xy = vel;
    rt_V.z = nrm.a;         //NOW rt_V:[vel.x,  vel.y,  metallic,  _]
    float depth = linearizeDepth(gl_FragCoord.z , 0.1f , 2000.0f);
    rt_P = vec4(wP, depth);  // 4;
}