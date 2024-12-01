#version 460 core
#include "common.glsl"
layout (location = 2) in vec3 N;
layout (location = 5) in vec2 uv;

layout(set=0, binding=2) uniform sampler2D colorTex;

layout(location=0) out vec4 outColor;
void main(){
    vec4 color = texture(colorTex, uv);
    if(color.a < 0.9)
        discard;
    color = gammaCorrect(color,2.2);
    outColor = vec4(color.rgb,0);
}