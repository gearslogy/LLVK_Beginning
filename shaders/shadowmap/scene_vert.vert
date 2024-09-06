#version 460 core
#include "gltf_layout_vert.glsl"
#include "math.glsl"

layout (binding = 0) uniform UBO
{
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 lightSpace; // 注意C++这里已经给设置了 灯光projMatrix,所以值会落到  NDC区域
    vec4 lightPos;
    float zNear;
    float zFar;
} ubo;


void main(){
    vec4 worldPos = ubo.model * vec4(P,1.0);
    gl_Position = ubo.projection * ubo.view * worldPos;

    mat3 normalMatrix =  normal_matrix(ubo.model);
    fragPosition = worldPos.xyz;
    fragTexCoord = uv0;
    fragN =  normalize(normalMatrix* N);
    fragTangent = normalize(normalMatrix * T);
    fragBitangent = normalize(normalMatrix  * B );
    fragColor = Cd;

}