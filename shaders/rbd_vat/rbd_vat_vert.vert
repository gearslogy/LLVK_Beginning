/*
glm::vec3 P{};      // 0
glm::vec3 Cd{};     // 1
glm::vec3 N{};      // 2
glm::vec3 T{};      // 3
glm::vec2 uv0{};    // 4
glm::int32_t fractureIndex{}; //5*/

#version 460 core

layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec3 T;
layout(location=5) in vec2 uv0;
layout(location=5) in int fracture_idx;


layout(location = 0) out vec3 fragN;
layout(location = 1) out vec2 fragTexCoord;


layout( binding=0) uniform UBO{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec4 time; // x for frame
}ubo;

layout(set=1, binding=1) uniform sampler2D positionVAT;
layout(set=1, binding=2) uniform sampler2D orientVAT;

void main(){
    ivec2 dimensions = textureSize(positionVAT, 0);
    int width = dimensions.x;  // num of points
    int height = dimensions.y; // vat frames

    // 计算纹理采样坐标
    // x: 根据顶点ID在行中的位置 (0-399)
    // y: 根据当前帧在总帧数中的位置 (0-59)
    vec2 vatST = vec2(
        (fracture_idx + 0.5) / float(width),    // 加0.5确保采样中心对齐
        (currentFrame + 0.5) / float(height) );

    vec4 posVatTex = texture( positionVAT, vatST);
    vec4 orientVatTex = texture( orientVAT, vatST);


    vec4 worldPos = ubo.proj * ubo.view* ubo.model * vec4(P, 1.0);
    gl_Position = worldPos;
    fragTexCoord = uv0;
    fragN = N;
}