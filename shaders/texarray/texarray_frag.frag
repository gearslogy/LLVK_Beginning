#version 460 core

layout (location = 0) in vec3 uv;
layout (set=1, binding = 0) uniform sampler2DArray tex2d;
layout (location = 0) out vec4 outFragColor;

void main(){
    vec3 tex = texture(tex2d, uv).rgb;
    tex = pow(tex, vec3(1/2.2));
    outFragColor = vec4(tex,1.0);
}