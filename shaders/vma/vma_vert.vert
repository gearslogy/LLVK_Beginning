#version 460 core
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor; //The location keyword is required in vulkan, but not in opengl
layout(location = 1) out vec3 fragN;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragBaseColor;
layout(location = 4) out vec3 fragSpecularColor;

layout(set=0, binding=0) uniform surface{
    vec4 baseColor;
    vec4 specularColor;
}ubo;


void main(){
    fragTexCoord = inTexCoord;
    gl_Position = vec4(P, 1.0);
    fragColor = Cd;
    fragBaseColor = ubo.baseColor.rgb;
    fragSpecularColor = ubo.specularColor.rgb;
}