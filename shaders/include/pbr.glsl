struct Light{
    vec4 position;
    vec3 color;
    float radius;
};


vec3 pbr(vec3 P, vec3 camPos, vec3 N, vec3 albedo, float roughness, float metallic, Light light) {
    vec3 V = normalize(camPos - P);

    vec3 lightPosition = light.position.xyz;
    vec3 lightColor = light.color;
    float lightRadius = light.radius;

    // 光照方向
    vec3 L = normalize(lightPosition - P);

    // 检查光源是否在表面可见侧
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0); // 光源在表面背面，不贡献光照

    vec3 H = normalize(V + L);

    // 光照衰减
    float distance = length(lightPosition - P);
    float attenuation = clamp(1.0 - (distance * distance) / (lightRadius * lightRadius), 0.0, 1.0);
    vec3 radiance = lightColor * attenuation;

    // 计算BRDF项
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = mix(vec3(0.04), albedo, metallic); // 基础反射率
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // Cook-Torrance BRDF
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    // 能量守恒
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    // 最终着色
    vec3 diffuse = kD * albedo / PI;
    return (diffuse + specular) * radiance * NdotL;
}


vec3 pbr2(vec3 P, vec3 camPos, vec3 N, vec3 albedo, float roughness, float metallic, Light light) {
    vec3 V = normalize(camPos - P);
    vec3 L = normalize(light.position.xyz - P);
    vec3 H = normalize(V + L);

    // 关键修改：树叶特殊光照处理
    float frontLight = max(dot(N, L), 0.0);
    float backLight = max(-dot(N, L), 0.0) * 0.4; // 背面透光率40%
    float NdotL = frontLight + backLight;

    // 光照衰减计算
    float distance = length(light.position.xyz - P);
    float attenuation = clamp(1.0 - (distance * distance) / (light.radius * light.radius), 0.0, 1.0);
    vec3 radiance = light.color * attenuation;

    // 标准PBR计算
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(frontLight, 0.001) + 0.001;
    vec3 specular = numerator / denominator;

    // 能量守恒
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    // 漫反射与环境光
    vec3 diffuse = kD * albedo / PI;


    // 对前向光使用完整PBR，对背向光使用简化模型
    vec3 frontResult = (diffuse + specular) * radiance * frontLight;
    vec3 backResult = (albedo * 0.5) * radiance * backLight; // 简化的背光模型

    return frontResult + backResult;
}