#version 460 core
#include "gltf_layout_frag.glsl"
layout (location=0) out vec4 outColor;
layout (binding = 1) uniform sampler2DArray tex2d; // albedo<0>/normal<1>/roughness<2>/displacement<3>/opacity<4>/translucency<5>

layout(push_constant) uniform PushConstantsFragment {
    float enable_opacity_texture;
} pcf;

void main(){

    if(pcf.enable_opacity_texture > 0){
        vec3 uv  = vec3(fragTexCoord, 4 );
        float alpha = texture(tex2d, uv).r;
        if(alpha<0.1) discard;
    }
    outColor = vec4(fragColor,1);
}