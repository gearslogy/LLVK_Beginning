#version 460 core
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;

// vulkan中location 必须要写。opengl不用写
layout(location = 0) out vec3 fragColor;


void main(){
    gl_Position = vec4(P,1.0);
    fragColor = Cd;
}