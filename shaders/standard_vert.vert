#version 460 core
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec2 inTexCoord;


layout(location = 0) out vec3 fragPosition; // World space position
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragN;        // Transformed normal
layout(location = 3) out vec2 fragTexCoord;




layout(set=0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

void main(){
    vec4 worldPos = ubo.model * vec4(P,1.0);
    gl_Position =  ubo.proj * ubo.view * worldPos;

    fragPosition = P;
    fragTexCoord = inTexCoord;
    fragN = mat3(transpose(inverse(ubo.model))) * N;
    fragColor = Cd;

}