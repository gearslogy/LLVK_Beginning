#version 460 core
#include "math.glsl"
layout (location=0) in vec3 P;
layout (location=1) in vec3 N;
layout (set=0, binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
    mat4 model;

    mat4 invView;
}ubo;

layout (location=0) out vec3 camP;
layout (location=1) out vec3 camN;

void main(){
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(P, 1.0);
    gl_Position = pos;
    // camera space variable
    camN = ubo.view * normalMatrix(ubo.model) * N;
    camP = ubo.view * ubo.model * P;

}