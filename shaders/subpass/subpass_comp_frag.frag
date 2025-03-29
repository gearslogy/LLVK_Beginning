#version 460 core
#include "common.glsl"
layout(location = 0) in vec2 uv;


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


struct Light{
    vec4 position;
    vec3 color;
    float radius;
};

layout (input_attachment_index = 0, binding = 1) uniform subpassInput sp_albedo;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput sp_NR; // x-y-z roughness
layout (input_attachment_index = 2, binding = 3) uniform subpassInput sp_VM;
layout (input_attachment_index = 3, binding = 4) uniform subpassInput sp_PD;   // x-y-z-lineardepth
layout(set=0 , binding = 5) buffer LightsBuffer{
    Light lights[];
};


vec3 reconstructWorldPositionFromDepth(float depth, vec2 texCoords) {
    vec3 ndcPos = vec3(texCoords * 2.0 - 1.0, depth); // 1. 重建 NDC 空间坐标
    // 2. 转换到视图空间
    vec4 viewPos = inverse(ubo.proj) * vec4(ndcPos, 1.0);
    viewPos /= viewPos.w;
    vec3 worldPos = (inverse(ubo.view) * viewPos).xyz;   // 3. 转换到世界空间
    return worldPos;
}

vec3 reconstructNormal(vec2 NXY){
    vec2 fenc = NXY * 2.0 - 1.0;    // 将 [0,1] 范围的值重映射到 [-1,1]
    float z = sqrt(1.0 - dot(fenc, fenc));    // 重建 Z 分量
    return normalize(vec3(fenc.x, fenc.y, z));    // 构建完整的法线向量
}

layout(location = 0) out vec4 swapchain;

void main(){
    // Read G-Buffer values from previous sub pass
    vec4 DIFF = subpassLoad(sp_albedo);         // [diff.x diff.y diff.z alpha]
    vec4 NR = subpassLoad(sp_NR);                 // [N.x| N.y | N.z | roughness ]
    vec4 VM = subpassLoad(sp_VM);                 // [v.x| v.y | metallic | none]
    vec4 subpassPData = subpassLoad(sp_PD);       // [P.x P.y P.z linear_depth]
    vec3 fragPos = subpassPData.xyz;

    // Ambient part
    vec3 camPos = ubo.cameraPosition.xyz;
    vec3 P =  fragPos;
    vec3 result = vec3(0);
    vec3 N = NR.xyz;
    float roughness = clamp(NR.w,0,1);
    float metallic = clamp(VM.b, 0, 1)*0.35;//clamp(VM.b,0,1);
    vec3 albedo = DIFF.rgb;

    for (int i=0; i < lights.length(); ++i)
    {
        vec3  lightPosition = lights[i].position.xyz;
        vec3  lightColor    = lights[i].color;
        float lightRadius   = lights[i].radius;

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
        result += color;
    }
    //result = result / (result + vec3(1.0)); // 简单的Reinhard色调映射
    float testN = dot(N, vec3(1,0,0));
    vec3 testCd = vec3(testN);
    swapchain = vec4(result , 1.0);
}
