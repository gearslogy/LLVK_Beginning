#version 460 core
// opengl can do without the "location" keyword
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragN;
layout(location = 2) in vec2 fragTexCoord;

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;

// uniform texture
layout(set=1, binding = 0) uniform sampler2D AlbedoTexSampler;
layout(set=1, binding = 1) uniform sampler2D DisplaceTexSampler;
layout(set=1, binding = 2) uniform sampler2D NormalTexSampler;
layout(set=1, binding = 3) uniform sampler2D OpacticyTexSampler;
layout(set=1, binding = 4) uniform sampler2D RoughnessTexSampler;
layout(set=1, binding = 5) uniform sampler2D TranslucencyTexSampler;

float near = 0.1;
float far  = 10.0;
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main(){
    float alpha = texture(OpacticyTexSampler, fragTexCoord).r;
    if(alpha<0.1) discard;

    float headLight = clamp(dot(fragN, normalize(vec3(0.0,1.0,0.0) ) ), 0, 1);

    vec3 trans = texture(TranslucencyTexSampler, fragTexCoord).rgb;
    trans = pow(trans, vec3(1.0/2.2))  ;
    vec3 tex = texture(AlbedoTexSampler, fragTexCoord).rgb ;
    tex = pow(tex, vec3(1.0/2.2))  ;

    vec3 transEffect = trans * tex * headLight + tex;


    outColor = vec4(transEffect, 1.0f);
}