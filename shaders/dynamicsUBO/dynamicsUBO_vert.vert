#version 460 core
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPosition; // World space position
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragN;        // Transformed normal
layout(location = 3) out vec2 fragTexCoord;



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
    gl_Position = uboView.projection * modelView * vec4(P.xyz, 1.0);

    // out
    fragTexCoord = inTexCoord;
    fragN = mat3(transpose(inverse(uboInstance.model))) * N;
    fragColor = Cd;
    vec3 worldPos = vec3( uboInstance.model * vec4(P, 1.0));
    fragPosition = worldPos;
}
