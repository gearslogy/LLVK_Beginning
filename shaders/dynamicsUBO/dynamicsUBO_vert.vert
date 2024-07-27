#version 460 core
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec2 inTexCoord;


layout(location = 0) out vec3 fragColor; //The location keyword is required in vulkan, but not in opengl
layout(location = 1) out vec3 fragN;
layout(location = 2) out vec2 fragTexCoord;



layout (set=0, binding = 0) uniform UboView
{
    mat4 projection;
    mat4 view;
} uboView;

layout (set=0, binding = 1) uniform UboInstance
{
    mat4 model;
} uboInstance;


void main()
{
    mat4 modelView = uboView.view * uboInstance.model;
    vec3 worldPos = vec3(modelView * vec4(P, 1.0));
    gl_Position = uboView.projection * modelView * vec4(P.xyz, 1.0);
    fragTexCoord = inTexCoord;
    fragN = N;
    fragColor = Cd;
}
