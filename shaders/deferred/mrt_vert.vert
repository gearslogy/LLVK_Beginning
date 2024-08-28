#version 460 core
layout (set=0, binding=0) uniform UBOData {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 instancePos[3];
    vec4 instanceRot[3];
    float instanceScale[3];
}ubo;

layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec3 T;
layout(location=4) in vec3 B;
layout(location=5) in vec2 uv0;
layout(location=6) in vec2 uv1;



layout(location = 0) out vec3 fragPosition; // World space position
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragN;        // Transformed normal
layout(location = 3) out vec3 fragTangent;        // Transformed normal
layout(location = 4) out vec3 fragBitangent;        // Transformed normal
layout(location = 5) out vec2 fragTexCoord;


void main(){
    vec4 worldPos = vec4(P,1) + ubo.instancePos[gl_InstanceIndex];
    gl_Position = ubo.proj * ubo.view * ubo.model * worldPos;

    fragPosition = worldPos.xyz;
    fragTexCoord = uv0;
    fragN =  normalize(N);
    fragTangent = normalize(T);
    fragBitangent = B;
    fragColor = Cd;

}