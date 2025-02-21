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
} ubo;

vec3 instance_dir(vec3 dir, vec4 orient){
    return normalize(rotateVectorByQuat(dir, orient) );
}


vec3 getCameraPosition(mat4 view) {
    mat4 invView = inverse(view);
    return vec3(invView[3][0], invView[3][1], invView[3][2]);
}

// XZ rotation
vec4 lookAtQuat(vec3 from, vec3 to) {
    vec3 forward = normalize(to - from);           // 从实例到摄像机的方向
    vec3 refForward = vec3(0.0, 0.0, 1.0);        // 默认朝向（Z 轴）

    // 只在 XZ 平面旋转，忽略 Y 分量
    vec3 projForward = normalize(vec3(forward.x, 0.0, forward.z));
    vec3 projRefForward = normalize(vec3(refForward.x, 0.0, refForward.z));

    float dotProd = dot(projRefForward, projForward);
    vec3 axis = cross(projRefForward, projForward);
    float angle = acos(clamp(dotProd, -1.0, 1.0));

    // 四元数计算
    float s = sin(angle * 0.5);
    return vec4(axis * s, cos(angle * 0.5));
}

void main(){
    vec3 cameraPos = getCameraPosition(ubo.view);
    // S R T order
    mat3 instance_scale_matrix = scale(vec3(instance_scale));
    vec3 instance_scaled_p = instance_scale_matrix * P;
    vec3 instance_rotated_p = rotateVectorByQuat(instance_scaled_p, instance_orient);

    vec3 localP = instance_rotated_p;
    // make billboard face camera
    vec4 billboard_orient = lookAtQuat(localP, cameraPos);
    localP = rotateVectorByQuat(localP, billboard_orient);

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
