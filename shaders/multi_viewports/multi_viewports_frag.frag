#version 460 core

layout(location=0) in INPUT{
    vec3 N;
    vec3 T;
    vec2 uv0;
};

layout(set=0, binding=1) uniform sampler2D baseSampler;
layout(set=0, binding=2) uniform sampler2D NRSampler;

layout(location=0) out vec4 outColor;


void main(){
    vec4 base = texture(baseSampler, uv0);
    vec4 NR   = texture(NRSampler, uv0);
    if (base.r < 0.2)
        discard;
    outColor = vec4(base.rgb, 0);
}