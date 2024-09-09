#version 460 core
#include "gltf_layout_frag.glsl"

layout (binding = 1) uniform sampler2DArray maps; // albedo<0>/normal<1>/roughness<2>/displacement<3>/opacity<4>/translucency<5>
layout (binding = 2) uniform sampler2D shadowMap;

// OUT
layout (location = 0) out vec4 outColor;

void main(){
    outColor = vec4(1,0,0,0);
}