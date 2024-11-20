#version 460 core
#include "common.glsl"
layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;
void main(){
    outColor = vec4(vec3(1,0,0),1);
}