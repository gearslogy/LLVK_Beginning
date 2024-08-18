#version 460 core
#include "common.glsl"
#include "math.glsl"
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec3 T;
layout(location=4) in vec3 B;
layout(location=5) in vec2 uv0;
layout(location=6) in vec2 uv1;
// instance locations
layout(location=7) in vec3 instance_p;
layout(location=8) in vec3 instance_rot;
layout(location=9) in vec3 instance_scale;
layout(location=10) in int instance_tex_idx;

layout (binding = 0) uniform UBO{
    mat4 proj;
    mat4 model_view;
    float global_rotation; // all star-field rotation
}camera_ubo;

// to frag variables
layout(location = 0) out vec3 fragPosition; // World space position
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragN;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;
layout(location = 5) out vec3 fragTexCoord; // vector UV


void main()
{
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
