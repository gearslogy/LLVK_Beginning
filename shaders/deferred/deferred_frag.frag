#version 460 core

layout (location = 0) in vec2 uv;



struct Light {
    vec4 position;
    vec3 color;
    float radius;
};

layout (set=0, binding = 0) uniform UBO
{
    Light lights[6];
    vec4 viewPos;
    int displayDebugTarget;
} ubo;


layout(set=1, binding = 0) uniform sampler2D PositionTexSampler;
layout(set=1, binding = 1) uniform sampler2D NormalTexSampler;
layout(set=1, binding = 2) uniform sampler2D AlbedoTexSampler;
layout(set=1, binding = 3) uniform sampler2D RoughnessTexSampler;
layout(set=1, binding = 4) uniform sampler2D DisplaceTexSampler;


// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;
void main(){
    vec4 result = vec4(texture(AlbedoTexSampler, uv).rgb ,1);
    result = pow(result, vec4(1/2.2));
    outColor = result;
}