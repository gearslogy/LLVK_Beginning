#version 460 core
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;



layout(location = 0) out vec3 fragColor; //The location keyword is required in vulkan, but not in opengl


layout(binding = 0) uniform MVP{
    vec3 dynamicsColor;
    mat4 model;
    mat4 view;
    mat4 proj;
}camera;

layout(binding = 0) uniform SURFACE{
    float baseAmp;
    float specularAmp;

    vec4 base;
    vec4 specular;
    vec4 normal;
    vec3 aa;
    vec4 bb;
}surface;


void main(){
    gl_Position = vec4(P,1.0);
    //fragColor = surface.base.rgb;
    fragColor = vec3(surface.specularAmp,0,0 );
}