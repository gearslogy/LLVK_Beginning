#version 460 core
#include "gltf_layout_frag.glsl"
#include "common.glsl"
layout(location=6) in vec3 in_L; // world space vertexP to lightP
layout(location=7) in vec4 in_shadow_uv;
layout(location=8) in vec4 bias_in_shadow_uv;

layout (set=1, binding = 0) uniform sampler2D albedo;
layout (set=1, binding = 1) uniform sampler2D ordp;
layout (set=1, binding = 2) uniform sampler2D shadowMap;

// OUT
layout (location = 0) out vec4 outColor;

float opengl_textureProj(vec4 shadow_coord, vec2 off){
    float shadow = 1;
    float bias = 0.173;
    vec3 projCoords = in_shadow_uv.xyz / in_shadow_uv.w;
    projCoords = projCoords*0.5 + 0.5;
    // 不知道为什么，用这个线性化depth判断才可以判断出来
    float realz = LinearizeDepth(projCoords.z, 0.1, 1000 ) / 1000;
    float close_dist = texture(shadowMap, projCoords.st).r;
    close_dist = LinearizeDepth( close_dist,0.1, 1000) / 1000;
    if( close_dist < realz - bias)  {
        shadow = 0;
    }
    return shadow;
}

float textureProj(vec4 shadow_coord, vec2 off)
{
    float bias =  0.000001;
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
    ivec2 texDim = textureSize(shadowMap, 0);
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

void main(){

    vec4 albedo = texture(albedo, fragTexCoord);
    float alpha = albedo.a;
    if(alpha<0.1) discard;


    float shadow = filterPCF(bias_in_shadow_uv);
    albedo *= shadow;
    outColor = vec4(albedo.rgb,1);


    //float shadow = opengl_textureProj(in_shadow_uv,vec2(0));
    //albedo *= shadow;
    //outColor = vec4(albedo,1);

}