#version 460 core
#include "common.glsl"
layout (location=0) in vec3 uv;


layout(set=0, binding=1) uniform samplerCube diffTex;

layout (location = 0) out vec4 outFragColor;
void main(){
    vec3 color = texture(diffTex, uv).rgb;
    color = gammaCorrect(color,2.2);
    outFragColor = vec4(color,0);
}