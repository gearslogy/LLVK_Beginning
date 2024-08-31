#version 460 core
#include "common.glsl"
layout (location = 0) in vec2 uv;



struct Light {
    vec4 position;
    vec3 color;
    float radius;
};

layout (set=0, binding = 0) uniform UBO
{
    Light lights[2];
    vec4 viewPos;
} ubo;


layout(set=1, binding = 0) uniform sampler2D PositionTexSampler;
layout(set=1, binding = 1) uniform sampler2D NormalTexSampler;
layout(set=1, binding = 2) uniform sampler2D AlbedoTexSampler;
layout(set=1, binding = 3) uniform sampler2D RoughnessTexSampler;
layout(set=1, binding = 4) uniform sampler2D DisplaceTexSampler;


// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;
void main(){
    vec3 albedo = texture(AlbedoTexSampler, uv).rgb;
    vec3 N = texture(NormalTexSampler, uv).rgb;
    //N = gammaCorrect(N,1.0/2.2); // because we are VK_FORMAT_R8G8B8A8_UNORM
    //N = normalCorrect(N);
    vec3 P = texture(PositionTexSampler, uv).rgb;
    float roughness = texture(RoughnessTexSampler, uv).r;
    float displace = texture(DisplaceTexSampler, uv).r;
    float metallic = 0;

    #define light_count 2
    vec3 cameraPosition = ubo.viewPos.xyz;

    vec3 result = vec3(0);
    for (int i=0; i < light_count; ++i)
    {
        vec3  lightPosition = ubo.lights[i].position.xyz;
        vec3  lightColor     = ubo.lights[i].color;
        float lightRadius    = ubo.lights[i].radius;

        vec3 V = normalize(cameraPosition - P);
        vec3 L = normalize(lightPosition - P);
        vec3 H = normalize(V + L);


        // Light attenuation
        float distance = length(lightPosition - P);
        float attenuation = clamp(1.0 - (distance * distance) / (lightRadius * lightRadius), 0.0, 1.0);
        vec3 radiance = lightColor * attenuation  ;
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
        // For energy conservation, the diffuse and specular light can't be above 1.0 (unless the surface emits light)
        vec3 kD = vec3(1.0) - kS;
        // If the material is metallic, then it doesn't have diffuse component
        kD *= 1.0 - metallic;
        // Lambertian BRDF
        vec3 diffuse = albedo / 3.14159265359;
        // Calculate radiance (light intensity)
        float NdotL = max(dot(N, L), 0.0);
        float ndotl_temp = max(dot(N, L), 0.0);
        radiance *= NdotL;
        // Combine diffuse and specular contribution
        vec3 color = (kD * diffuse + specular) * radiance;
        result += color;
    }

    outColor = vec4(result,1);



}