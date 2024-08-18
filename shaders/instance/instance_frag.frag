#version 460 core

layout(location = 0) in vec3 fragPosition; // World space position
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragN;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;
layout(location = 5) in vec3 fragTexCoord;


// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;


// uniform texture. in feature we should be diff + N + ordp texture.
layout(set=1, binding = 0) uniform sampler2D AlbedoTexSampler;
layout(set=1, binding = 1) uniform sampler2D AOTexSampler;
layout(set=1, binding = 2) uniform sampler2D DisplaceTexSampler;
layout(set=1, binding = 3) uniform sampler2D NormalTexSampler;
layout(set=1, binding = 4) uniform sampler2D RoughnessTexSampler;


void main(){
    outColor = vec4(1,0,0,1);
}