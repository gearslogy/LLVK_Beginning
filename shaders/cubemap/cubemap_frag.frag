#version 460 core
layout (locatin=0) in vec3 uv;


layout(set=0, binding=0) uniform samplerCube diffTex;

layout (location = 0) out vec4 outFragColor;
void main(){
    outFragColor = texture(diffTex, uv);
}