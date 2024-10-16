#version 460 core
#include "gltf_layout_frag.glsl"
#include "common.glsl"
layout(location=6) in vec3 in_L; // world space vertexP to lightP
layout(location=7) in vec4 in_shadow_uv;
layout(location=8) in vec4 in_bias_shadow_uv;

layout (set=1, binding = 0) uniform sampler2DArray albedo_array;
layout (set=1, binding = 1) uniform sampler2DArray ordp_array;
layout (set=1, binding = 2) uniform sampler2DArray n_array;

// attachment out
layout (location = 0) out vec4 outColor;


void main(){
    outColor =  vec4(1,0,0,1);
}



