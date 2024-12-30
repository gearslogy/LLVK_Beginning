#version 460 core
layout (location=0) in vec3 P;
layout (set=0, binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
}ubo;

layout(location=0) out vec3 uv;

void main(){
    uv = normalize(P);
    gl_Position = projection * view * vec4(P, 1.0);
}