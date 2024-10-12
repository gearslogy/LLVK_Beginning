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
    mat3 instance_scale_matrix = scale(instance_scale);
    vec3 instance_scaled_p = instance_scale_matrix * P;
    vec3 instance_rotated_p = rotateVectorByQuat(instance_scaled_p, instance_orient);
    vec3 instance_transformed_p = instance_rotated_p + instance_p;

    // normal changed
    mat3 nm = normal_matrix(instance_scale_matrix);
    fragN = nm * N;
    fragTangent = nm * T;
    fragBitangent = nm * B;


    mat3 xrot = rotate_x(radians(instance_rot.x));
    mat3 yrot = rotate_y(radians(instance_rot.y));
    mat3 zrot = rotate_z(radians(instance_rot.z));
    mat3 inst_r  = zrot *  yrot * xrot;
    mat3 inst_s  = scale(instance_scale);

    mat3 glob_rot = rotate_y(radians(camera_ubo.global_rotation));

    mat3 inst_r_s = inst_r * inst_s; //  R S matrix
    vec3 worldP = glob_rot * (inst_r_s * P + instance_p);

    mat3 n_mat = normal_matrix(glob_rot * inst_r_s);

    // apply mvp
    gl_Position = camera_ubo.proj * camera_ubo.model_view * vec4(worldP , 1.0);

    // var out
    fragPosition = gl_Position.xyz; // World space position
    fragColor;
    fragN = n_mat * N;
    fragTangent = n_mat * T;
    fragBitangent = n_mat * B;
    fragTexCoord = vec3(uv0, instance_tex_idx);
}
