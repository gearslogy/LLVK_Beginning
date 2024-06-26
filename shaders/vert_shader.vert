#version 460 core
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec2 inTexCoord;


layout(location = 0) out vec3 fragColor; //The location keyword is required in vulkan, but not in opengl
layout(location = 1) out vec3 fragN;
layout(location = 2) out vec2 fragTexCoord;



layout(set=0, binding = 0) uniform UniformBufferObject {
    vec2 screenSize;
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

layout(set=0, binding = 1) uniform SURFACE{
    float baseAmp;
    float specularAmp;

    vec4 base;
    vec4 specular;
    vec4 normal;
    vec3 aa;
    vec4 bb;
}surface;


void main(){
    gl_Position =  ubo.proj * ubo.view * ubo.model  *vec4(P,1.0);
    fragTexCoord = inTexCoord;
    fragN = N;
    fragColor = Cd;
}