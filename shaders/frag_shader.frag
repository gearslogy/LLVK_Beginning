#version 460 core
// OGL中，作为从vertex shader传递过来的一律不用写location
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

// OGL 中，如果输出附件就一个 可以不用写location
layout (location = 0) out vec4 outColor;

// uniform texture
layout(binding = 2) uniform sampler2D texSampler;
void main(){
    outColor = vec4(fragTexCoord,0,1.0f);
    vec3 tex = texture(texSampler, fragTexCoord).rgb;
    tex = pow(tex, vec3(1.0/2.2)); // gamma校正将把线性颜色空间转变为非线性空间
    //outColor =  vec4(tex, 1.0);
}