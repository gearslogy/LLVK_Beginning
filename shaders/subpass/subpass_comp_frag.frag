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

layout (set=0, input_attachment_index = 0, binding = 1) uniform subpassInput sp_albedo;
layout (set=0, input_attachment_index = 1, binding = 2) uniform subpassInput sp_NR; // x-y-z roughness
layout (set=0, input_attachment_index = 2, binding = 3) uniform subpassInput sp_VM;
layout (set=0, input_attachment_index = 3, binding = 4) uniform subpassInput sp_PD;   // x-y-z-lineardepth
layout (set=0 , binding = 5) buffer LightsBuffer{
    Light lights[];
};
layout (set=0,binding = 6) uniform sampler2D shadowMap;
layout (set=0,binding = 7) uniform LIGHTPROJ{
    mat4 mat;
}lightViewProj;


vec3 reconstructWorldPositionFromDepth(float depth, vec2 texCoords) {
    vec3 ndcPos = vec3(texCoords * 2.0 - 1.0, depth); // 1. 重建 NDC 空间坐标
    // 2. 转换到视图空间
    vec4 viewPos = inverse(ubo.proj) * vec4(ndcPos, 1.0);
    viewPos /= viewPos.w;
    vec3 worldPos = (inverse(ubo.view) * viewPos).xyz;   // 3. 转换到世界空间
    return worldPos;
}


const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0
);

float textureProj(vec4 shadow_coord, vec2 off)
{
    float bias =  0.00004;
    vec4 shadowCoord = shadow_coord / shadow_coord.w;
    float shadow = 1.0;
    if ( shadowCoord.z > 0 && shadowCoord.z < 1.0 ) //没必要判断.z>-1
    {
        float closeDepth = texture( shadowMap, shadowCoord.st + off ).r;
        float currentDepth = shadowCoord.z;
        if ( shadowCoord.w > 0.0 && closeDepth < (currentDepth - bias ) ){
            shadow = 0;
        }
    }
    return shadow;
}

float filterPCF(vec4 sc)
{
    ivec2 texDim = textureSize(shadowMap, 0); // MIP = 0
    float scale = .5;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 4;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
            count++;
        }
    }
    return shadowFactor / count;
}

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}


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

    vec3 N = NR.xyz;
    float roughness = clamp(NR.w,0,1);
    float metallic = clamp(VM.b, 0, 1)*0.7;//clamp(VM.b,0,1);
    vec3 albedo = DIFF.rgb;

    Light keylight = lights[0]; // keylight: sun light
    vec3 sunLightContrib = pbr(P,camPos, N, albedo, roughness , metallic, keylight );
    // -------- 计算keyLight阴影 -----------
    // 1. 首先把世界坐标转换到灯光空间
    vec4 shadow_wpToLight = lightViewProj.mat * vec4(fragPos, 1.0);
    vec4 shadow_lightToUV = biasMat * shadow_wpToLight;
    float shadow = clamp(filterPCF(shadow_lightToUV),0,1);

    // 其他灯光的贡献，不计算阴影

    vec3 result = vec3(0.0);
    for(int i=1;  i< lights.length(); ++i){
        result +=  pbr(P,camPos, N, albedo,  roughness , metallic, lights[i] );
    }
    result += sunLightContrib * shadow;    // 仅对这个光源应用阴影


    // . Tonemapping（ACES/Reinhard/Unreal等）
    vec3 tonemapped = ACESFilm(result); // 映射到 [0, 1]
    // . Gamma
    tonemapped  = gammaCorrect(tonemapped,2.2);

    swapchain = vec4(tonemapped , 1.0) ;
}
