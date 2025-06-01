#version 460 core
#extension GL_ARB_viewport_array : enable

layout(constant_id = 0 ) const int VIEWPORTS_NUM = 2;
layout (triangles, invocations = 2) in;
layout (triangle_strip, max_vertices = 3) out;

layout (set=0, binding=0) uniform UBO{
    mat4 proj[2];      // 两个视窗
    mat4 view[2];      // 两个视窗
    vec4 camPos[2];    // 两个摄像机位置
    vec4 keyLightPos;
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
    vec3 out_B;
    vec3 out_worldP;
    vec3 out_camPos;
    vec2 out_uv0;
};

layout(push_constant) uniform PushConstant {
    mat4 model;
    mat4 preModel;
} pcv;

void main(){
    for(int i=0; i <gl_in.length(); i++){
        out_N = gs_in[i].N;
        out_T = gs_in[i].T;
        out_B = normalize(cross(out_N, out_T) );
        out_uv0 = gs_in[i].uv0;
        out_camPos = ubo.camPos[gl_InvocationID].xyz;

        // 变换position -> P * V * M * p
        vec4 pos = gl_in[i].gl_Position;
        vec4 worldPos = pcv.model * pos;
        out_worldP = worldPos.xyz;
        gl_Position = ubo.proj[gl_InvocationID] *  ubo.view[gl_InvocationID] * worldPos;

        // important! viewport
        gl_ViewportIndex = gl_InvocationID;

        gl_PrimitiveID = gl_PrimitiveIDIn;
        EmitVertex();
    }
    EndPrimitive();
}