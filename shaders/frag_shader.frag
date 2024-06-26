#version 460 core
// opengl can do without the "location" keyword
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragN;
layout(location = 2) in vec2 fragTexCoord;

// opengl can do without the "location" keyword
layout (location = 0) out vec4 outColor;

// uniform texture
layout(set=1, binding = 0) uniform sampler2D texSampler;

float near = 0.1;
float far  = 10.0;
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}


layout(set=0, binding = 0) uniform UniformBufferObject {
    vec2 screenSize;
    mat4 model;
    mat4 view;
    mat4 proj;

}ubo;

void main(){
    outColor = vec4(fragTexCoord,0,1.0f);
    vec3 tex = texture(texSampler, fragTexCoord).rgb;

    float depth = LinearizeDepth(gl_FragCoord.z) / far;

    //tex = mix(tex,vec3(depth), phase);
    //phase = float(gl_FragCoord.x > 400);
    //tex = mix(tex,fragN, phase);
    //float headLight = dot(fragN, normalize(vec3(0.5,0.5,0.8) ) );
    //headLight = clamp(headLight, 0.2, 1);
    tex = pow(tex, vec3(1.0/2.2)) ;

    // mix N
    float phase = float( gl_FragCoord.x > ubo.screenSize.x * .5);
    //tex = mix(tex,fragN, phase);

    // mix depth
    //float phase2 = float( gl_FragCoord.x > ubo.screenSize.x * 0.666);
    //tex = mix(tex,vec3(depth), phase2);

    outColor =  vec4(tex , 1.0) ;
}