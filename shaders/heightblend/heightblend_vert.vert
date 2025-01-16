#version 460 core

layout(location = 0) in vec3 P;
layout(location = 1) in vec3 N;
layout(location = 2) in vec3 T;
layout(location = 3) in vec2 uv0;


layout (set=0, binding=0) uniform BLOCK{
    mat4 proj;
    mat4 view;
    mat4 model;
}ubo;

layout(location = 0) out vec2 o_uv;
layout(location = 1) out vec3 o_N;
layout(location = 2) out vec3 o_T;
layout(location = 3) out vec3 o_B;
layout(location = 4) out vec3 o_wP;



void main(){
    vec4 worldP = ubo.model * vec4(P,1);
    gl_Position = ubo.proj * ubo.view * worldP;
    o_uv = uv0;
    o_N  = N;
    o_T  = T;
    o_B  = normalize(cross(N,T) );
    o_wP = worldP.xyz;
}