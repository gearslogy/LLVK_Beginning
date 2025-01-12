
#version 460 core
layout (location = 0) in vec3 P;
layout (location = 1) in vec3 N;

layout(set=0, binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
    mat4 model;
}ubo;

layout(location = 0) out vec3 oN;

void main(){
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(P,1.0);
    oN = normalize(N);
}