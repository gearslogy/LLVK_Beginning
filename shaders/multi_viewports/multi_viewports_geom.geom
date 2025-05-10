#version 460 core
#extension GL_ARB_viewport_array : enable

layout(constant_id = 0 ) const int VIEWPORTS_NUM = 2;
layout(triangles, invocations = 2) in;
layout(triangle_strip, max_vertices = 3 ) out;

layout (set=0, binding=0) uniform UBO{
    mat4 proj[2];
    mat4 modelView[2];
    vec4 lightPos;
}ubo;

// 输入
layout(location=0) in INPUT{
    vec3 N;
    vec3 T;
    vec2 uv0;
}gs_in[];

// 输出
layout(location=0) out OUTPUT{
    vec3 out_N;
    vec3 out_T;
    vec2 out_uv0;
};

void main(){
    for(int i=0; i <gl_in.length(); i++){
        out_N = mat3(ubo.modelView[gl_InvocationID]) * gs_in[i].N;
        out_T = mat3(ubo.modelView[gl_InvocationID]) * gs_in[i].T;
        out_uv0 = gs_in[i].uv0;

        // 变换position -> P * V * M * p
        vec4 pos = gl_in[i].gl_Position;
        vec4 worldPos = ubo.modelView[gl_InvocationID] * pos;
        gl_Position = ubo.proj[gl_InvocationID] * worldPos;

        // important! viewport
        gl_ViewportIndex = gl_InvocationID;

        gl_PrimitiveID = gl_PrimitiveIDIn;
        EmitVertex();
    }
    EndPrimitive();
}