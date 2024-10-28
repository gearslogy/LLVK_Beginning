#version 460 core
#include "gltf_layout_vert.glsl" // out: P Cd N T B uv
#include "math.glsl"

layout(location = 6) out vec3 L;  // world space vertexP to lightP
layout(location = 7) out vec4 uvShadow;
layout(location = 8) out vec4 bias_uvShadow;

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0 );
// UBO
layout (set=0, binding = 0) uniform UBO
{
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 lightSpace;
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

    L =  normalize(ubo.lightPos.xyz - worldPos.xyz);
    // 把当前世界位置转换到灯光空间 world P -> light P

    //
    uvShadow =  ubo.lightSpace * worldPos;
    bias_uvShadow = biasMat * uvShadow; // [-1,1] ---> [0,1],因为采样贴图必须0-1 相当于*.5 然后再加0.5

}