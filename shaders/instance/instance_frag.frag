#version 460 core
#include "gltf_layout_frag.glsl"

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;


// uniform texture. in feature we should be diff + N + ordp texture.
layout(set=1, binding = 0) uniform sampler2D albedoTex;
layout(set=1, binding = 1) uniform sampler2D ordpTex;
layout(set=1, binding = 2) uniform sampler2D NTex;


void main(){
    vec2 uv = fragTexCoord;
    vec4 albedo = texture(albedoTex, uv);
    float alpha = albedo.r;
    if(alpha < 0.1)
        discard;
    outColor = vec4(1,0,0,1);
}