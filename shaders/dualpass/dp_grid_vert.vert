#version 460 core

#include "gltf_layout_vert.glsl"
#include "math.glsl"
layout ( set=0, binding=0) uniform UBOData {
    mat4 proj;
    mat4 view;
    mat4 model;
}ubo;

void main(){
    vec4 worldPos = ubo.model * vec4(P,1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    fragTexCoord = uv0;
}