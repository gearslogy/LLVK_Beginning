#version 460 core
#include "common.glsl"

layout(location = 0) in vec3 wN;
layout(location = 1) in vec3 wT;
layout(location = 2) in vec3 wB;
layout(location = 3) in vec3 wP;
layout(location = 4) in vec2 uv;


// binding = 0 is UBO
layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerV;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput samplerP;

layout(location = 0) out vec4 outColor;
void main(){
    // Sample depth from deferred depth buffer and discard if obscured
    float depth = subpassLoad(samplerP).a;
    float cDepth = linearizeDepth(gl_FragCoord.z , 0.1f , 2000.0f);
    if ((depth != 0.0) && (cDepth > depth)) {
        discard;
    };

    outColor = vec4(1,0,0,1);
}