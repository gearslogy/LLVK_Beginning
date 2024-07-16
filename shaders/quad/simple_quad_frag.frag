#version 460 core
// opengl can do without the "location" keyword
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragN;
layout(location = 2) in vec2 fragTexCoord;

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;

// uniform texture
layout(set=1, binding = 0) uniform sampler2D texSampler;

void main(){
    outColor =  vec4(fragTexCoord.xy, 0 , 1.0) ;
}