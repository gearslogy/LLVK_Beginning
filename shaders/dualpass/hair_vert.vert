#version 460 core

#include "gltf_layout_vert.glsl"
#include "math.glsl"
layout ( set=0, binding=0) uniform UBOData {
    mat4 proj;
    mat4 view;
    mat4 model;
}ubo;
layout(location = 6) out vec2 fragUV1;

void main(){
    vec4 worldPos = ubo.model * vec4(P,1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    fragTexCoord = uv0;
    fragN = normalize(N);
    fragUV1 = uv1;
}