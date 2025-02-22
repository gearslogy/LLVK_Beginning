/*
glm::vec3 P{};      // 0
glm::vec3 Cd{};     // 1
glm::vec3 N{};      // 2
glm::vec3 T{};      // 3
glm::vec2 uv0{};    // 4
glm::int32_t fractureIndex{}; //5*/

#version 460 core
#include "math.glsl"


layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec3 T;
layout(location=4) in vec2 uv0;
layout(location=5) in int fracture_idx;


layout(location = 0) out vec3 fragN;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragCd;
layout(location = 3) out vec3 fragVAT_P;
layout(location = 4) out vec4 fragVAT_orient;

layout(set=0, binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec4 metaInfo; // x for frame,y is num fractures
}ubo;


struct RBDData{
    vec4 rbdP;
    vec4 rbdOrient;
};

layout(set=1, binding=1) buffer SSBO{
    RBDData data[];  // numRBDS * numFrames
}ssbo;

void main(){
    int evalFrame = int(ubo.metaInfo.x);
    int numFractures = int(ubo.metaInfo.y);
    int dataIndex = fracture_idx + numFractures * evalFrame;
    RBDData packData = ssbo.data[dataIndex];
    vec4 packP = packData.rbdP;
    vec4 packOrient = packData.rbdOrient;

    // Cd is RBD pivot
    vec3 toPivotSpace = P - Cd;
    vec3 rbdP = qrotate(toPivotSpace,  packOrient) +  packP.xyz;
    vec4 worldPos = ubo.proj * ubo.view* ubo.model * vec4(rbdP, 1.0);
    gl_Position = worldPos;
    fragTexCoord = uv0;
    vec3 rbdN =  qrotate(N,  packOrient);
    fragN = normalize(rbdN );
    fragCd = Cd;
    fragVAT_P = packP.xyz;
    fragVAT_orient = packOrient;
}