#version 460 core

layout (location = 2) in vec3 N;
layout (location = 5) in vec2 uv;

layout(binding=0) uniform sampler2D colorTex;

void main(){
    vec4 color = texture(colorTex, uv);
    if(color.a < 0.9)
        discard;
}