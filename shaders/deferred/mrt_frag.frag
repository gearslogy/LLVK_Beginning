#version 460 core
#include "gltf_layout_frag.glsl"
#include "common.glsl"
layout(set=1, binding = 0) uniform sampler2D AlbedoTexSampler;
layout(set=1, binding = 1) uniform sampler2D NormalTexSampler;
layout(set=1, binding = 2) uniform sampler2D RoughnessTexSampler;
layout(set=1, binding = 3) uniform sampler2D DisplaceTexSampler;

// attachment output
layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
layout (location = 3) out vec4 outRoughness;
layout (location = 4) out vec4 outDisplace;

void main(){
    vec4 albedo = texture(AlbedoTexSampler, fragTexCoord);
    vec4 normal = texture(NormalTexSampler, fragTexCoord);
    vec3 correct_normal = normalCorrect(normal.rgb);
    vec4 rough  = texture(RoughnessTexSampler, fragTexCoord);
    vec4 disp  = texture(DisplaceTexSampler, fragTexCoord);

    mat3 TBN = mat3(fragTangent, fragBitangent, fragN);
    vec3 transformN = normalize(TBN * correct_normal.rgb);

    outPosition = vec4(fragPosition,1);
    //outNormal   = vec4(fragTangent,1);
    outNormal = vec4(transformN , 1);
    outAlbedo   = albedo;
    outRoughness = rough;
    outDisplace = disp;
}