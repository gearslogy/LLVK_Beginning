#version 460 core
#include "common.glsl"
layout (location = 0) in vec2 uv;

layout(set=0, binding = 0) uniform sampler2D diffuseMap;

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;
void main(){
    vec4 incoming = texture( diffuseMap, uv);
    outColor = incoming;
}