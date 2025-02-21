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
void main(){
    vec4 diff =texture(diffSampler, vertexIn.uv0);
    diff = gammaCorrect(diff,2.2);
    if(diff.a < 0.1)discard;

    //color = diff;
    color = vec4(vertexIn.index, 0,0,0 );
}