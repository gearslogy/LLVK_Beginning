#version 460 core
#include "common.glsl"
#include "math.glsl"
#include "gltf_layout_vert.glsl" // 0-6 P ... uv0 uv1

// instance locations
layout(location=7) in vec3  instance_p;
layout(location=8) in vec4  instance_orient;
layout(location=9) in float instance_scale;

layout (set=0, binding = 0) uniform UBO
{
    mat4 proj;
    mat4 view;
    mat4 model; // all instance transform not nesscery! ignore it keep it simple
    mat4 lightSpace;
    vec4 lightPos;
    float zNear;
    float zFar;
} ubo;

vec3 instance_dir(vec3 dir, vec4 orient){
    return normalize(qrotate(dir, orient) );
}

void main()
{
    // S R T order
    mat3 instance_scale_matrix = scale(vec3(instance_scale));
    vec3 instance_scaled_p = instance_scale_matrix * P;
    vec3 instance_rotated_p = qrotate(instance_scaled_p, instance_orient);
    vec3 instance_transformed_p = instance_rotated_p + instance_p;

    // normal changed
    mat3 nm = normal_matrix(instance_scale_matrix);


    // apply mvp
    gl_Position = ubo.proj * ubo.view * vec4(instance_transformed_p , 1.0);

    // var out
    fragN = nm * N;
    fragN = instance_dir(fragN, instance_orient);
    fragTangent = nm * T;
    fragTangent = instance_dir(fragTangent, instance_orient);
    fragBitangent = nm * B;
    fragBitangent = instance_dir(fragBitangent, instance_orient);
    fragPosition = gl_Position.xyz; // World space position
    fragTexCoord = uv0;
}
