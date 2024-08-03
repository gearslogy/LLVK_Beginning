#version 460 core
layout(location=0) in vec3 P;
layout(location=1) in vec3 Cd;
layout(location=2) in vec3 N;
layout(location=3) in vec3 T;
layout(location=4) in vec3 B;
layout(location=5) in vec2 uv0;
layout(location=6) in vec2 uv1;


layout(location = 0) out vec3 fragPosition; // World space position
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragN;        // Transformed normal
layout(location = 3) out vec3 fragTangent;        // Transformed normal
layout(location = 4) out vec3 fragBitangent;        // Transformed normal
layout(location = 5) out vec2 fragTexCoord;




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
    mat3 normalMatrix = mat3(transpose(inverse(uboInstance.model)));
    gl_Position = uboView.projection * modelView * vec4(P.xyz, 1.0);

    // out
    fragTexCoord = uv0;
    fragN =  normalMatrix* N;
    fragTangent = T;
    fragBitangent = B;
    fragColor = Cd;
    vec3 worldPos = vec3( uboInstance.model * vec4(P, 1.0));
    fragPosition = worldPos;
}
