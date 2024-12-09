#version 460 core
#include "gltf_layout_vert.glsl"
#include "math.glsl"
layout(constant_id = 0 ) const int enableInstance = 0;

layout ( set=0, binding=0) uniform UBOData {
    mat4 proj;
    mat4 view;
    mat4 model;
    vec4 instancePos[4]; // 29-geometry will using this position
}ubo;

layout(location = 6) out vec4 outCameraP;
void main(){
    vec4 worldPos = vec4(0);
    if(enableInstance == 1){
        vec3 instance_pos   = ubo.instancePos[gl_InstanceIndex].xyz;
        vec3 finalPosition = P + instance_pos;
        worldPos = vec4(finalPosition, 1);
        gl_Position = ubo.proj * ubo.view * ubo.model * worldPos;
    }
    else{
        worldPos = ubo.model * vec4(P, 1);
        gl_Position =  ubo.proj * ubo.view * worldPos;;
    }
    fragPosition = worldPos.xyz;
    fragN = normal_matrix(ubo.model)* normalize(N);
    fragTexCoord = uv0;
    outCameraP = ubo.view * worldPos;

}