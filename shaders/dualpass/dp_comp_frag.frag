#version 460 core
#include "common.glsl"
layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D diffuseMap;
void main(){
    vec4 color = texture(diffuseMap, uv);
    outColor = color;
}