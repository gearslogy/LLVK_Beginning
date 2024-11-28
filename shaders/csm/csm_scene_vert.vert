#version 460 core
#include "gltf_layout_vert.glsl"

layout(constant_id = 0 ) const int enableInstance = 0;

layout ( set=0, binding=0) uniform UBOData {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 instancePos[4]; // 29-geometry will using this position
}ubo;


void main(){
    if(enableInstance){
        vec3 instance_pos   = ubo.instancePos[gl_InstanceIndex].xyz;
        vec3 finalPosition = P + instance_pos;
        vec4 worldPos = vec4(finalPosition, 1);
        gl_Position = ubo.proj * ubo.view * ubo.model * worldPos;
        fragPosition = worldPos;
        fragN = normalize(N);
        fragTexCoord = uv0;
    }
    else{
        vector worldPos =  ubo.proj * ubo.view * ubo.model * vec4(P, 1);
        gl_Position = worldPos;
        fragPosition = worldPos;
        fragN = normalize(N);
        fragTexCoord = uv0;
    }
}