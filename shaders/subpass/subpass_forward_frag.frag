#version 460 core
#include "common.glsl"

layout(location = 0) in vec3 wN;
layout(location = 1) in vec3 wT;
layout(location = 2) in vec3 wB;
layout(location = 3) in vec3 wP;
layout(location = 4) in vec2 uv;

struct Light{
    vec4 position;
    vec3 color;
    float radius;
};

layout(set=0 , binding =0 ) uniform UBO{
    mat4 proj;
    mat4 view;

    mat4 preProj;
    mat4 preView;
    vec4 cameraPosition;
}ubo;


// binding = 0 is UBO
layout (binding = 1) uniform sampler2D AlbedoTexSampler;
layout (binding = 2) uniform sampler2D NRMTexSampler;
layout (input_attachment_index = 0, binding = 3) uniform subpassInput samplerV;
layout (input_attachment_index = 1, binding = 4) uniform subpassInput samplerP;
layout (set=0 , binding = 5) buffer LightsBuffer{
    Light lights[];
};


vec3 pbr(vec3 P,vec3 camPos, vec3 N, vec3 albedo, float roughness , float metallic, Light light ){
    vec3  lightPosition = light.position.xyz;
    vec3  lightColor    = light.color;
    float lightRadius   = light.radius;

    // 这行应该在循环外计算一次，因为V对每个光源都是相同的
    vec3 V = normalize(camPos - P);
    vec3 L = normalize(lightPosition - P);
    vec3 H = normalize(V + L);

    // Light attenuation
    float distance = length(lightPosition - P);
    float attenuation = clamp(1.0 - (distance * distance) / (lightRadius * lightRadius), 0.0, 1.0);
    vec3 radiance = lightColor * attenuation;

    // 计算BRDF项
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = vec3(0.04); // Fresnel reflectance at normal incidence for non-metal
    F0 = mix(F0, albedo, metallic); // metallic objects use albedo as F0
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // Cook-Torrance BRDF
    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // prevent divide by zero
    vec3 specular = nominator / denominator;

    // kS is equal to Fresnel
    vec3 kS = F;
    // For energy conservation, the diffuse and specular light can't be above 1.0
    vec3 kD = vec3(1.0) - kS;
    // If the material is metallic, then it doesn't have diffuse component
    kD *= 1.0 - metallic;

    // Lambertian BRDF
    vec3 diffuse = albedo / PI; // 使用常量PI而不是硬编码值更清晰

    // 计算NdotL一次就够了，不需要重复计算
    float NdotL = max(dot(N, L), 0.0);

    // 将NdotL应用到最终辐射度上
    // 这里不要将NdotL再乘到radiance上，因为它已经在最终BRDF公式中使用了
    vec3 color = (kD * diffuse + specular) * radiance * NdotL;
    return color;
}
vec3 reconstructNormal(vec2 NXY){
    vec2 fenc = NXY * 2.0 - 1.0;    // 将 [0,1] 范围的值重映射到 [-1,1]
    float z = sqrt(1.0 - dot(fenc, fenc));    // 重建 Z 分量
    return normalize(vec3(fenc.x, fenc.y, z));    // 构建完整的法线向量
}
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}


layout(location = 0) out vec4 outColor;
void main(){
    vec2 uv0 = uv * 2;
    vec4 base = texture(AlbedoTexSampler, uv0);
    vec4 nrm = texture(NRMTexSampler, uv0);
    nrm = gammaCorrect(nrm,2.2);

    float depth = subpassLoad(samplerP).a;
    float cDepth = linearizeDepth(gl_FragCoord.z , 0.1f , 2000.0f);
    if ((depth != 0.0) && (cDepth > depth)) {
        discard;
    };


    vec3 normalmap = reconstructNormal(nrm.xy);
    mat3 TBN = mat3(wT, wB, wN);
    vec3 N = normalize(TBN * normalmap);


    vec3 albedo = base.rgb;
    float roughness = nrm.b;
    float metallic = 0;
    vec3 camPos = ubo.cameraPosition.xyz;
    vec3 P = wP;

    vec3 result = vec3(0.0);
    for(int i=0;  i< lights.length(); ++i){
        result +=  pbr(P,camPos, N, albedo,  roughness , metallic, lights[i] );
    }
    // . Tonemapping（ACES/Reinhard/Unreal等）
    vec3 tonemapped = ACESFilm(result); // 映射到 [0, 1]
    // . Gamma
    tonemapped  = gammaCorrect(tonemapped,2.2);
    vec3 L = normalize((lights[0].position.xyz-P) );
    float test = max(dot(L,wT),0);
    vec3 test3= vec3(test);
    outColor = vec4(tonemapped,0.66);
}