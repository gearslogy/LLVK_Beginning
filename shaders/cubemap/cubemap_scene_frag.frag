#version 460 core
#include "common.glsl"
layout (location=0) in vec3 camP; // camera to object vector
layout (location=1) in vec3 camN;

layout (set=0, binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
    mat4 model;

    mat4 invView;
}ubo;
layout(set=0, binding=1) uniform samplerCube cubeTex;


layout (location=0) out vec4 outFragCd;
void main(){
    vec3 I = normalize(camP); // cam to object
    vec3 N = normalize(camN);
    vec3 R = normalize(reflect(I, N) );
    R.x*=-1;
    vec3 worldR = vec3(ubo.invView * vec4(R,0));
    const int lodBias=  0;
    vec4 color = texture(cubeTex, worldR, lodBias); // 世界空间采样颜色
    color = gammaCorrect(color,2.2);

    float fresnel = 1-max(dot(N, -I),0);
    color = mix(vec4(0.6,0.2,0.2,1), color, fresnel);
    outFragCd =vec4(color);
}