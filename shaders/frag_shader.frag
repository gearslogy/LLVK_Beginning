#version 460 core
// opengl can do without the "location" keyword
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;

// uniform texture
layout(set=1, binding = 0) uniform sampler2D texSampler;

void main(){
    outColor = vec4(fragTexCoord,0,1.0f);
    vec3 tex = texture(texSampler, fragTexCoord).rgb;
    tex = pow(tex, vec3(1.0/2.2));
    outColor =  vec4(tex, 1.0);
}