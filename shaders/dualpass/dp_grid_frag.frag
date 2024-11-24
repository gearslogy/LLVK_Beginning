#version 450
#include "common.glsl"
layout(location = 2) in vec3 N;
layout(location = 5) in vec2 inTexCoord;

// 材质参数
layout(binding = 1) uniform sampler2D diffuseMap;
layout (location = 0) out vec4 outColor;
void main() {
    vec4 color = texture(diffuseMap, inTexCoord);
    color = gammaCorrect(color,2.2);
    float top = dot (N, vec3(0,1,0));
    top= max(top, 0.2 );
    outColor = vec4(color.rgb * top, 0);
}