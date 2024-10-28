#version 460 core
#include "gltf_layout_frag.glsl"
#include "common.glsl"
layout(location=6) in vec3 in_L; // world space vertexP to lightP
layout(location=7) in vec4 in_shadow_uv;
layout(location=8) in vec4 in_bias_shadow_uv;

layout (set=1, binding = 0) uniform sampler2DArray albedo_array; // cliff1/cliif2/small_rock/road/grass
layout (set=1, binding = 1) uniform sampler2DArray ordp_array;
layout (set=1, binding = 2) uniform sampler2DArray n_array;
layout (set=1, binding = 3) uniform sampler2D terrain_mask;

// attachment out
layout (location = 0) out vec4 outColor;

float randomNoise(vec2 coord) {
    return fract(sin(dot(coord.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float easeOutCubic(float number) {
    return 1 - pow(1 - number, 3);
}

void main(){
    float view_depth = linearizeDepth(gl_FragCoord.z , 0.1f , 1500.0f) / 1500.0f;
    view_depth = 1 - view_depth;
    float depth_bias = mix(0.8, 1, easeOutCubic(view_depth));
    depth_bias = pow(depth_bias , 4);
    vec4 mask = texture(terrain_mask, fragTexCoord);
    vec2 uv = fragTexCoord * 80 ;

    // 采样每个纹理层
    vec4 tex0 = texture(albedo_array, vec3(uv, 0)); // cliff1
    vec4 tex1 = texture(albedo_array, vec3(uv, 1)); // cliff2
    vec4 tex2 = texture(albedo_array, vec3(uv, 2)); // small rocks
    vec4 tex3 = texture(albedo_array, vec3(uv, 3)); // road
    vec4 tex4 = texture(albedo_array, vec3(uv, 4)); // grass

    // 采样 ordpArray 获取 displacement 通道
    vec4 ordp0 = texture(ordp_array, vec3(uv, 0));
    vec4 ordp1 = texture(ordp_array, vec3(uv, 1));
    vec4 ordp2 = texture(ordp_array, vec3(uv, 2));
    vec4 ordp3 = texture(ordp_array, vec3(uv, 3));

    // 使用 mask 混合纹理层
    vec4 diff = tex4;
    diff = mix(diff, tex0, mask.r);
    diff = mix(diff, tex1, mask.g);
    diff = mix(diff, tex2, mask.b);
    diff = mix(diff, tex3, mask.a);


    float top_light = dot(fragN, normalize(vec3(0,0.5,1)) );
    top_light = clamp(top_light,0,1);
    diff *= top_light;
    diff = gammaCorrect(diff, 2.2)*1.25;

    outColor =  vec4(vec3(diff),1);

}



