#version 460 core
#include "common.glsl"

layout (location=0) in vec3 P;

layout(set=0, binding=1) uniform sampler2D diffTex;

layout (location = 0) out vec4 outFragColor;

void main(){
    vec2 fragUV = directionToSphericalUV(P);
    vec3 color = texture(diffTex, fragUV).rgb;
    color = ACESToneMapping(color);
    outFragColor = vec4(color,0);
}