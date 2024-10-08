#version 460 core
#include "gltf_layout_frag.glsl"

// foliage layers:6  [albedo<0>/normal<1>/roughness<2>/displacement<3>/opacity<4>/translucency<5>]
// opaque  layers:5  [albedo<0>/normal<1>/roughness<2>/displacement<3>/AO]
layout (binding = 1) uniform sampler2D albedo;

void main(){
    float alpha = texture(albedo, fragTexCoord).r;
    if(alpha<0.1) discard;
}