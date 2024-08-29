#version 460 core
layout (std140, set=0, binding=0) uniform UBOData {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 instancePos[3];
    vec4 instanceRot[3];
    vec4 instanceScale[3];

}ubo;
#include "gltf_layout_vert.glsl"

void main(){
    float instance_scale = ubo.instanceScale[gl_InstanceIndex].x;
    vec3  instance_pos   = ubo.instancePos[gl_InstanceIndex].xyz;
    // 应用缩放
    vec3 P_scaled = P * instance_scale;
    // 应用位移
    vec3 finalPosition = P_scaled + instance_pos;
    vec4 worldPos = vec4(finalPosition, 1);
    gl_Position = ubo.proj * ubo.view * ubo.model * worldPos;

    fragPosition = worldPos.xyz;
    fragTexCoord = uv0;
    fragN =  normalize(N);
    fragTangent = normalize(T);
    fragBitangent = B;
    fragColor = Cd;

}