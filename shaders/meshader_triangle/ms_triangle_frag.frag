#version 460 core

//layout (location =0 ) in vec4 color;

layout (location= 0 ) in VertexInput{
    vec4 color;
}vertexInput;

layout(location = 0)  out vec4 outColor;

void main(){
    outColor = vertexInput.color;
}