#version 460 core
// -- IN --
layout(location=0) in vec3 P;
layout(location=1) in vec3 N;
layout(location=2) in vec3 T;
layout(location=3) in vec2 uv0;

// -- OUT --
layout(location=0) out vec2 uv;

// the binding at 3
layout (binding=3) uniform UBO{
    mat4 depth_mvp;
}ubo;

/*
layout(push_constant) uniform PushConstant {
    mat4 model;
    mat4 preModel;
} pcv;*/


void main(){
    vec4 worldPos = ubo.depth_mvp * vec4(P, 1.0);
    gl_Position = worldPos;
}