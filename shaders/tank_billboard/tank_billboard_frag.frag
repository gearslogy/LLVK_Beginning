#version 460 core
#include "common.glsl"
layout(location=0) out vec4 color;
layout(location = 0) in VertexOut {
    vec2 uv0;
    vec2 uv1;
    vec2 uv2;
    vec2 uv3;
    flat int index;
} vertexIn;

layout(binding=1) uniform sampler2D diffSampler;
layout(binding=2) uniform sampler2D transSampler;
void main(){
    vec4 diff =texture(diffSampler, vertexIn.uv0);
    vec4 trans =texture(transSampler, vertexIn.uv0);

    if(diff.a < 0.5)discard;
    diff = gammaCorrect(diff,2.2) * max(trans.r,0.5) * 1.2;
    color = vec4(diff.rgb, 0);
}