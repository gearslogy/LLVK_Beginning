#version 460 core
#include "common.glsl"
layout(location = 0) in vec3 N;
layout(location = 1) in vec2 uv0;
layout(location = 2) in vec3 Cd;
layout(location = 3) in vec3 fragVAT_P;
layout(location = 4) in vec4 fragVAT_orient;

layout(set=1, binding=0) uniform sampler2D diffTex;


layout (location = 0) out vec4 outColor;

void main(){
    vec4 diff = texture( diffTex, uv0);
    diff = gammaCorrect(diff,2.2);
    float topLight = max(dot(N, vec3(0,1,0) ) , 0.3);

    outColor = vec4(vec3(diff) * topLight, 1);
    //outColor = vec4(fragVAT_P,1);
}