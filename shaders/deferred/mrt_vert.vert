#version 460 core
layout ( set=0, binding=0) uniform UBOData {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 instancePos[3];
    vec4 instanceRot[3];
    vec4 instanceScale[3];

}ubo;
#include "gltf_layout_vert.glsl"
#include "math.glsl"
void main(){
    float instance_scale = ubo.instanceScale[gl_InstanceIndex].x;
    vec3  instance_pos   = ubo.instancePos[gl_InstanceIndex].xyz;
    vec3  instance_rot   = ubo.instanceRot[gl_InstanceIndex].xyz;

    // scale matrix
    mat3 scale_mat = scale(vec3(instance_scale) );
    mat3 rot_mat = right_hand_eulerAnglesToRotationMatrix(instance_rot);
    //mat3 rot_x = rotate_x( radians (instance_rot.x) );
    //mat3 rot_y = rotate_x( radians (instance_rot.y) );
    //mat3 rot_z = rotate_z( radians (instance_rot.z) );

    //mat3 rot_mat = rot_x;
    vec3 P_scaled = rot_mat * scale_mat * P;
    // 应用位移
    vec3 finalPosition = P_scaled + instance_pos;
    vec4 worldPos = vec4(finalPosition, 1);
    gl_Position = ubo.proj * ubo.view * ubo.model * worldPos;


    mat3 normal_mat = normal_matrix(rot_mat*scale_mat);
    fragPosition = worldPos.xyz;
    fragTexCoord = uv0;
    fragN =  normal_mat * normalize(N);
    fragTangent = normal_mat * normalize(T);
    fragBitangent = normal_mat * B;
    fragColor = Cd;

}