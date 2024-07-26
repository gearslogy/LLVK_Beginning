#version 460 core
// opengl can do without the "location" keyword
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragN;
layout(location = 2) in vec2 fragTexCoord;

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;

// uniform texture. in feature we should be diff + N + ordp texture.
layout(set=1, binding = 0) uniform sampler2D AlbedoTexSampler;
layout(set=1, binding = 1) uniform sampler2D AOTexSampler;
layout(set=1, binding = 2) uniform sampler2D DisplaceTexSampler;
layout(set=1, binding = 3) uniform sampler2D NormalTexSampler;
layout(set=1, binding = 4) uniform sampler2D RoughnessTexSampler;

float near = 0.1;
float far  = 10.0;
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}


void main(){
    outColor = vec4(fragTexCoord,0,1.0f);
}