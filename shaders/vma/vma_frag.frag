#version 460 core
// opengl can do without the "location" keyword
layout(location = 0) in vec3 fragColor; //The location keyword is required in vulkan, but not in opengl
layout(location = 1) in vec3 fragN;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragBaseColor;
layout(location = 4) in vec3 fragSpecularColor;

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;

// uniform texture
layout(set=1, binding = 0) uniform sampler2D texSampler;

void main(){
    outColor =  vec4(fragTexCoord.xy, 0 , 1.0) ;
    vec3 tex = texture(texSampler, fragTexCoord).rgb;
    outColor =  vec4(tex, 1.0) ;
}