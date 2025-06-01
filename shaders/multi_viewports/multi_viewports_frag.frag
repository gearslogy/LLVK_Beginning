#version 460 core
#include "common.glsl"
#include "shadowFn.glsl"
#include "pbr.glsl"
layout(location=0) in INPUT{
    vec3 wN;
    vec3 wT;
    vec3 wB;
    vec3 wP;
    vec3 camPos;
    vec2 uv0;
};

layout (set=0, binding=0) uniform UBO{
    mat4 proj[2];      // 两个视窗
    mat4 view[2];      // 两个视窗
    vec4 camPos[2];
    vec4 keyLightPos;
}ubo;
layout(set=0, binding=1) uniform sampler2D baseSampler;
layout(set=0, binding=2) uniform sampler2D NRSampler;
layout(set=0, binding=3) uniform LIGHTPROJ{
    mat4 mat;
}lightViewProj;
layout(set=0, binding=4) uniform sampler2D shadowMap;

layout(location=0) out vec4 swapchain;


vec3 reconstructNormal(vec2 NXY){
    vec2 fenc = NXY * 2.0 - 1.0;    // 将 [0,1] 范围的值重映射到 [-1,1]
    float z = sqrt(1.0 - dot(fenc, fenc));    // 重建 Z 分量
    return normalize(vec3(fenc.x, fenc.y, z));    // 构建完整的法线向量
}

void main(){
    vec4 base = texture(baseSampler, uv0);
    vec4 NR   = texture(NRSampler, uv0);

    if (base.a < 0.1)
        discard;

    // ---------------Key Light --------
    Light light;
    light.position = ubo.keyLightPos;
    light.color = vec3(1.0, 0.8175, 0.6) * 15;
    light.radius = 200.0;
    // ---------------Back Light ----------
    Light backLight;
    backLight.position = vec4(-57.6449,-18.9421,30.3312,1.0);
    backLight.color = vec3(0.631, 0.8155, 1) * 2;
    backLight.radius = 120.0;



    vec3 normalmap = reconstructNormal(NR.xy);
    mat3 TBN = mat3(wT, wB, wN);
    vec3 N = normalize(TBN * normalmap);


    //float rough = min(NR.b * 2 ,1);
    float rough = 0.7;
    float metallic = 0;




    // 把世界坐标转换到灯光空间
    vec4 shadow_wpToLight = lightViewProj.mat * vec4(wP, 1.0);
    vec4 shadow_lightToUV = biasMat * shadow_wpToLight;
    float shadow = clamp(filterPCF(shadowMap, shadow_lightToUV),0,1);
    vec3 pbrResult = pbr(wP, camPos, N, base.rgb, rough, metallic, light);
    pbrResult *= max(shadow, 0.0);

    vec3 ambient = base.rgb * 0.1; // 轻微环境光
    pbrResult+= ambient;

    vec3 pbrBackResult = pbr(wP, camPos, N, base.rgb, rough, metallic, backLight) * 0.2;
    pbrResult += pbrBackResult;

    // . Tonemapping（ACES/Reinhard/Unreal等）
    vec3 tonemapped = ACESToneMapping(pbrResult); // 映射到 [0, 1]
    // . Gamma
    tonemapped  = gammaCorrect(tonemapped,2.2);

    // -------- test ndotup -----------------
    float ndotup = dot(N, vec3(0,1,0));
    ndotup = max(ndotup, 0);
    // -------- test N -----------------------

    swapchain = vec4(vec3(tonemapped), 0);
}