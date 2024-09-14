#version 460 core
#include "gltf_layout_frag.glsl"


// foliage layers:6  [albedo<0>/normal<1>/roughness<2>/displacement<3>/opacity<4>/translucency<5>]
// opaque  layers:5  [albedo<0>/normal<1>/roughness<2>/displacement<3>/AO]
layout (binding = 1) uniform sampler2DArray maps;
layout (binding = 2) uniform sampler2D shadowMap;

layout(push_constant) uniform PushConstantsFragment {
    float enable_opacity_texture;
} pc_data;

// OUT
layout (location = 0) out vec4 outColor;

void main(){
    if(pc_data.enable_opacity_texture > 0.5){
        vec3 uv  = vec3(fragTexCoord, 4 );
        float alpha = texture(maps, uv).r;
        if(alpha<0.1) discard;
    }
    vec3 albedo_uv  = vec3(fragTexCoord, 0 );
    vec3 albedo = texture(maps, albedo_uv).rgb;

    vec3 shadow = texture(shadowMap, fragTexCoord).rgb;

    outColor = vec4(albedo,1);
}