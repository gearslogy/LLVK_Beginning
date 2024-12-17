#version 460 core
#include "common.glsl"
layout(location = 0) in vec3 N;
layout(location = 1) in vec2 uv0;

layout(set=1, binding=0) uniform sampler2D diffTex;


layout (location = 0) out vec4 outColor;

void main(){
    vec4 diff = texture( diffTex, uv0);
    diff = gammaCorrect(diff,2.2);
    outColor = vec4(diff.rgb, 1);
}