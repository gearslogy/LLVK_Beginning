#version 460 core
layout(location=0) in vec2 uv;
layout(binding=1) uniform sampler2D texSampler;
void main(){
    vec4 base = texture(texSampler, uv);
    if(base.a <0.1) discard;
}