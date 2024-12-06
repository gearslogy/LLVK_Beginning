#version 460 core
#include "math.glsl"

layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec3 T;
layout(location=4) in vec3 B;
layout(location=5) in vec2 uv0;
layout(location=6) in vec2 uv1;


// depth pass only rely on the uv
layout(location = 0) out vec2 out_uv0;

// constant var that use
layout(constant_id = 0) const int enableInstance = 0;

layout(set=0, binding=0) uniform UBOData{
    mat4 proj; // dropped
    mat4 view; // dropped
    mat4 model;// dropped.
    vec4 instancePos[4]; //29-geometry will using this position
}ubo;


void main() {
    if(enableInstance == 1){
        vec3 instanceP = P + ubo.instancePos[gl_InstanceIndex].xyz;
        gl_Position = vec4(instanceP, 1.0);// worldP->geom shader
        out_uv0 = uv0;// uv-> geom
    }
    else{
        gl_Position = vec4(P, 1.0);
        out_uv0 = uv0;
    }
}
