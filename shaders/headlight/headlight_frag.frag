#version 460 core


layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 N;
void main(){
    float lambert = max(dot(N,vec3(0,1,0)) , 0.3);
    outColor = vec4(vec3(lambert),0);
}