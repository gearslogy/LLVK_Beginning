#version 460 core
#include "common.glsl"
#include "math.glsl"

layout(location=0) in vec3 P;
layout(location=1) in vec3 N;
layout(location=2) in vec3 T;
layout(location=3) in vec2 uv0;
layout(location=4) in vec2 uv1;
layout(location=5) in vec2 uv2;
layout(location=6) in vec2 uv3;
layout(location=7) in int idx;
// instance data
layout(location=8) in vec3 instance_p;
layout(location=9) in vec4 instance_orient;
layout(location=10) in float instance_scale;


layout(location = 0) out VertexOut {
    vec2 o_uv0;  // location 0
    vec2 o_uv1;  // location 1
    vec2 o_uv2;  // location 2
    vec2 o_uv3;  // location 3
    flat int o_index;
} vertexOut;

layout(set=0, binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec4 camPos;
} ubo;

vec3 instance_dir(vec3 dir, vec4 orient){
    return normalize(qrotate(dir, orient) );
}


void main(){
    vec3 cameraPos = ubo.camPos.xyz;
    // S R T order
    mat3 instance_scale_matrix = scale(vec3(instance_scale));
    vec3 instance_scaled_p = instance_scale_matrix * P;
    vec3 instance_rotated_p = qrotate(instance_scaled_p, instance_orient);

    vec3 localP = instance_scaled_p;
    // make billboard face camera
    vec3 camDir = normalize(cameraPos - localP);
    camDir.y = 0;
    float angle = atan(camDir.x, camDir.z); // [-PI : PI ]
    float angle_view_degrees = degrees(angle);
    if(angle < 0) angle_view_degrees += 360;


    vec4 billboard_orient = quaternion(radians(angle_view_degrees),vec3(0,1,0));

    localP = qrotate(localP, billboard_orient);

    // to world space
    vec3 wP = localP + instance_p; // worldSpace position
    // apply mvp
    gl_Position = ubo.proj * ubo.view * vec4(wP , 1.0);

    vertexOut.o_uv0 = uv0;
    vertexOut.o_uv1 = uv1;
    vertexOut.o_uv2 = uv2;
    vertexOut.o_uv3 = uv3;
    vertexOut.o_index = idx;
}
