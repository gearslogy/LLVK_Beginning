
#version 460 core
#include "math.glsl"


layout (constant_id = 0) const int myConstant0 = 0;
layout (constant_id = 1) const float myConstant1 = 1.0;

layout(push_constant) uniform PushConstantsVertex {
    float P_xOffset;
    float P_yOffset;
    float P_zOffset;
    float P_wOffset;
} myPushConstantData;


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

layout(set=0, binding=1) uniform UBO2{
    vec4 time;
    vec4 color;
}ubo2;
layout(set=0, binding=2) uniform UBO3{
    vec4 data;
    vec4 noise;
}ubo3;

struct RBDData{
    vec4 rbdP;
    vec4 rbdOrient;
};

struct Light {
    vec4 position;
    vec3 color;
    float radius;
};
layout (set=0, binding = 3) uniform UBO4
{
    Light lights[10];
    vec4 viewPos;
} ubo4;

layout(set=1, binding=1) buffer SSBO{
    RBDData data[];  // numRBDS * numFrames
}ssbo;

layout(set=1, binding=2) uniform sampler2D testTex1;
layout(set=1, binding=3) uniform sampler2D testTex2;

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