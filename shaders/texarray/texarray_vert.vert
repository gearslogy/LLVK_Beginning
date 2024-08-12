#version 460 core
layout(location = 0) in vec3 P;
layout(location = 1) in vec3 Cd;
layout(location = 2) in vec3 N;
layout(location = 3) in vec2 uv;


struct Instance{
    mat4 model;
    float arrayIndex;
};

layout(set=0, binding = 0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
    Instance instance[4];
}ubo;

layout (location = 0) out vec3 outUV;


void main(){
    outUV = vec3(uv, ubo.instance[gl_InstanceIndex].arrayIndex);
    mat4 mv = ubo.view * ubo.instance[gl_InstanceIndex].model;
    gl_Position =  ubo.proj * mv  *vec4(P,1.0);
}