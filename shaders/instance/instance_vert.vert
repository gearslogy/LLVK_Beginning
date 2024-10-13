#version 460 core
#include "common.glsl"
#include "math.glsl"
#include "gltf_layout_vert.glsl" // 0-6 P ... uv0 uv1

// instance locations
layout(location=7) in vec3  instance_p;
layout(location=8) in vec4  instance_orient;
layout(location=9) in float instance_scale;

layout (set=0,binding = 0) uniform UBO{
    mat4 proj;
    mat4 model_view;
}camera_ubo;



void main()
{
    // S R T order
    mat3 instance_scale_matrix = scale(vec3(instance_scale));
    vec3 instance_scaled_p = instance_scale_matrix * P;
    vec3 instance_rotated_p = rotateVectorByQuat(instance_scaled_p, instance_orient);
    vec3 instance_transformed_p = instance_rotated_p + instance_p;

    // normal changed
    mat3 nm = normal_matrix(instance_scale_matrix);


    // apply mvp
    gl_Position = camera_ubo.proj * camera_ubo.model_view * vec4(instance_transformed_p , 1.0);

    // var out
    fragN = nm * N;
    fragTangent = nm * T;
    fragBitangent = nm * B;
    fragPosition = gl_Position.xyz; // World space position
    fragTexCoord = uv0;
}
