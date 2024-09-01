#version 460 core
#include "gltf_layout_vert.glsl"

layout (binding=0) uniform UBO{
    mat4 depth_mvp;
}ubo;


void main(){
    vec4 worldPos = ubo.depth_mvp * vec4(P, 1.0);
    gl_Position = worldPos;
    fragTexCoord = uv0;
 }