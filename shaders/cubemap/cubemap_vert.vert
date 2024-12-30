#version 460 core
layout (location=0) in vec3 P;
layout (set=0, binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
}ubo;

layout(location=0) out vec3 uv;

void main(){
    uv = normalize(P);
    uv.x *= -1.0;
    vec4 pos = ubo.proj * ubo.view * vec4(P, 1.0);
    gl_Position = pos.xyww;
}