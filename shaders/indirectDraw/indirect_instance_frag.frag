#version 460 core
#include "gltf_layout_frag.glsl"
#include "common.glsl"
// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;
layout (location = 6) in vec3 inUV;


layout(set=1, binding = 0) uniform sampler2DArray albedoTex;
layout(set=1, binding = 1) uniform sampler2DArray NTex;
layout(set=1, binding = 2) uniform sampler2DArray rmop; // rough metal ao

void main(){

    vec4 albedo = texture(albedoTex, inUV);
    float alpha = albedo.a;
    if(alpha < 0.3)
        discard;
    vec3 diff = gammaCorrect(albedo.rgb, 2.2);
    float top_light = dot(fragN, normalize(vec3(0,0.5,1)) );
    top_light = clamp(top_light,.5,1);
    diff *= top_light;
    outColor = vec4(diff,1);
}