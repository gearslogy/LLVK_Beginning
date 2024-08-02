#version 460 core
#include "common.glsl"
// opengl can do without the "location" keyword

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragN;
layout(location = 3) in vec2 fragTexCoord;

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;

// uniform texture
layout(set=1, binding = 0) uniform sampler2D AlbedoTexSampler;
layout(set=1, binding = 1) uniform sampler2D DisplaceTexSampler;
layout(set=1, binding = 2) uniform sampler2D NormalTexSampler;
layout(set=1, binding = 3) uniform sampler2D OpacticyTexSampler;
layout(set=1, binding = 4) uniform sampler2D RoughnessTexSampler;
layout(set=1, binding = 5) uniform sampler2D TranslucencyTexSampler;


void main(){
    vec3 tangentNormal = texture(NormalTexSampler, fragTexCoord).xyz * 2.0 - 1.0;
    vec3 N = getNormalFromMap(fragN,tangentNormal, fragPosition, fragTexCoord);
    float alpha = texture(OpacticyTexSampler, fragTexCoord).r;
    float metallic = 0;
    vec3 trans = texture(TranslucencyTexSampler, fragTexCoord).rgb;
    vec3 albedo  =  texture(AlbedoTexSampler, fragTexCoord).rgb;
    float roughness = texture(RoughnessTexSampler, fragTexCoord).r;
    float ao = 1;
    if(alpha<0.1) discard;


    vec3 V = normalize(cameraPosition - fragPosition);
    vec3 L = normalize(lightPosition - fragPosition);
    vec3 H = normalize(V + L);

    // Light attenuation
    float distance = length(lightPosition - fragPosition);
    float attenuation = clamp(1.0 - (distance * distance) / (lightRadius * lightRadius), 0.0, 1.0);
    vec3 radiance = lightColor  ;

    // Pre-calculated values
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
    float ndotl_temp = max(dot(fragN, L), 0.0);
    radiance *= NdotL;

    // Combine diffuse and specular contribution
    vec3 color = (kD * diffuse + specular) * radiance;

    // Apply ambient occlusion
    color *= ao;

    // Apply gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); // Assuming gamma correction with gamma = 2.2

    outColor = vec4(vec3(color), 1.0);
}