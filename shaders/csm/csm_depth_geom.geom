#version 460 core
#define cascade_count 4

layout(triangles, invocations = 4) in;
layout(triangle_strip, max_vertices = 3 ) out;

layout(location = 0) in vec2 uv0[];    // 接收顶点着色器输出的纹理坐标
layout(location = 0) out vec2 out_uv0; //

// Cascade矩阵UBO
layout(set = 0, binding = 1) uniform CascadeUBO {
    mat4 lightViewProj[cascade_count];
} ubo;

void main() {
    gl_Layer = gl_InvocationID;
    // 添加范围检查
    if(gl_InvocationID >= cascade_count) {
        return;
    }

    for(int i = 0; i < 3; i++) {
        gl_Position = ubo.lightViewProj[gl_InvocationID] * gl_in[i].gl_Position;
        out_uv0 = uv0[i];
        EmitVertex();
    }
    EndPrimitive();
}
