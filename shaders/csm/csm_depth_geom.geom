#version 460 core
#define cascade_count 4

layout(triangles) in;
layout(triangle_strip, max_vertices = 3 * cascade_count ) out;

layout(location = 0) in vec2 uv0[];    // 接收顶点着色器输出的纹理坐标
layout(location = 0) out vec2 out_uv0; //

// Cascade矩阵UBO
layout(set = 0, binding = 1) uniform CascadeUBO {
    mat4 lightViewProj[cascade_count];
} ubo;

void main() {
    // 对每个cascade层级输出一次三角形
    for(int cascadeIndex = 0; cascadeIndex < cascade_count; cascadeIndex++) {
        gl_Layer = cascadeIndex;        // 设置当前输出的层
        for(int i = 0; i < 3; i++) {        // 输出三角形的三个顶点
            // 应用对应cascade的视图投影矩阵
            gl_Position = ubo.lightViewProj[cascadeIndex] * gl_in[i].gl_Position;
            // 传递纹理坐标给片段着色器
            out_uv0 = uv0[i];
            EmitVertex();
        }
        EndPrimitive();
    }
}
