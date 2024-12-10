#version 460 core
#include "common.glsl"
#define cascade_count 4



#define ambient 0.35
layout (location = 0) in vec3 wP;
layout (location = 2) in vec3 N;
layout (location = 5) in vec2 uv;
layout (location = 6) in vec4 cameraP;
// Cascade矩阵UBO
layout(set = 0, binding = 1) uniform CascadeUBO {
    mat4 lightViewProj[cascade_count];
} casacdeViewProjMatrices;

layout(set=0, binding=2) uniform FsUBO{
    vec4 cascadeSplits; // vec4 is 4 cascade count
    vec4 lightDir;
}ubo;

layout(set=0, binding=3) uniform sampler2D colorTex;
layout(set=0, binding=4) uniform sampler2DArray depthTex;



const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);


float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
    float shadow = 1.0;
    float bias = 0.001;

    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
        float dist = texture(depthTex, vec3(shadowCoord.st + offset, cascadeIndex)).r;
        if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
            shadow = ambient;
        }
    }
    return shadow;
}

float filterPCF(vec4 sc, uint cascadeIndex)
{
    ivec2 texDim = textureSize(depthTex, 0).xy;
    float scale = 0.75;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);
    float shadowFactor = 0.0;
    int count = 0;
    int range = 3;

    for (int x = -range; x <= range; x++) {
        for (int y = -range; y <= range; y++) {
            shadowFactor += textureProj(sc, vec2(dx*x, dy*y), cascadeIndex);
            count++;
        }
    }
    return shadowFactor / count;
}


layout(location=0) out vec4 outColor;
void main(){
    vec4 color = texture(colorTex, uv);
    vec3 debugCascadeIndexCd = vec3(0);
    if(color.a < 0.9)
        discard;
    color = gammaCorrect(color,2.2);

    vec3 L = -normalize(ubo.lightDir).xyz;
    float diff = max(dot(N,L), ambient);

    // cal cascade index
    uint cascadeIndex = 0;
    for(int i = 0;i < cascade_count - 1 ; ++i){
        if(cameraP.z < ubo.cascadeSplits[i]) {
            cascadeIndex = i + 1;
        }
    }

    vec4 shadowCoord = biasMat * casacdeViewProjMatrices.lightViewProj[cascadeIndex] * vec4(wP,1.0);
    float shadow = filterPCF(shadowCoord, cascadeIndex);

    if(cascadeIndex == 0){
        debugCascadeIndexCd = vec3(1,0,0);
    }else if (cascadeIndex == 1){
        debugCascadeIndexCd = vec3(0,1,0);
    } else if (cascadeIndex == 2){
        debugCascadeIndexCd = vec3(0,0,1);
    }else{
        debugCascadeIndexCd = vec3(1,1,0);
    }

    vec3 finalColor =  debugCascadeIndexCd* color.rgb * vec3(shadow) * diff * 3.0;
    outColor = vec4(finalColor,0);
}