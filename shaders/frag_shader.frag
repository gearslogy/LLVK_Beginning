#version 460 core
// OGL中，作为从vertex shader传递过来的一律不用写location
layout (location = 0) in vec3 fragColor;

// OGL 中，如果输出附件就一个 可以不用写location
layout (location = 0) out vec4 outColor;

void main(){
    outColor = vec4(fragColor,1.0f);
}