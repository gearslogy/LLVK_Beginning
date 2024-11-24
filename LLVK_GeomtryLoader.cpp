//
// Created by liuya on 8/3/2024.
//
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "LLVK_GeomtryLoader.h"

#include <iostream>
#include "Utils.h"

LLVK_NAMESPACE_BEGIN

VkVertexInputBindingDescription GLTFVertex::bindings() {
    VkVertexInputBindingDescription desc{};
    desc.binding = vertex_buffer_binding_id;
    desc.stride = sizeof(GLTFVertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}

std::array<VkVertexInputAttributeDescription, 7> GLTFVertex::attribs() {
    std::array<VkVertexInputAttributeDescription,7> desc{};
    desc[0] = { 0,vertex_buffer_binding_id,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, P)};
    desc[1] = { 1,vertex_buffer_binding_id,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, Cd)};
    desc[2] = { 2,vertex_buffer_binding_id,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, N)};
    desc[3] = { 3,vertex_buffer_binding_id,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, T)};
    desc[4] = { 4,vertex_buffer_binding_id,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, B)};
    desc[5] = { 5,vertex_buffer_binding_id,VK_FORMAT_R32G32_SFLOAT , offsetof(GLTFVertex, uv0)};
    desc[6] = { 6,vertex_buffer_binding_id,VK_FORMAT_R32G32_SFLOAT , offsetof(GLTFVertex, uv1)};
    return desc;
}

void GLTFLoader::load(const std::string &path) {
    std::cout << "[[GLTFLoader::load]]:" << path << std::endl;
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!warn.empty()) {
        std::cerr << "Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "Error: " << err << std::endl;
    }
    if (!ret) {
        std::cerr << "Failed to load GLTF file" << std::endl;
    }
    filePath = path;
    // OUR LIB ONLY SUPPORT HOUDINI shopmaterial_path. so model.meshes must be ==1
    assert(model.meshes.size()==1);
    const auto &mesh = model.meshes[0]; // I found houdini name attribute will build multi meshes. but now I just want to get single model, single mesh, multi primitives
    parts.resize(mesh.primitives.size());
    for (auto &&[part_id,primitive]: UT_Fn::enumerate(mesh.primitives)) {
        // if this object has two materials, there are two primitives.
        auto &part = parts[part_id];
        std::cout << "primitive indices:" << primitive.indices << std::endl; //
        const float *positions = getAttribPointer<float>(model, primitive, "POSITION");

        bool hasNormal = (primitive.attributes.find("NORMAL") != primitive.attributes.end());
        bool hasTangent = (primitive.attributes.find("TANGENT") != primitive.attributes.end());
        bool hasUV0 = (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end());
        bool hasUV1 = (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end());
        bool hasCd0 = (primitive.attributes.find("COLOR_0") != primitive.attributes.end());
        // fix attribute rendering!
        const float *N = nullptr;
        const float *T = nullptr;
        const float *uv0 = nullptr;
        const float *uv1 = nullptr;
        const float *Cd = nullptr;

        if (hasNormal)
            N = getAttribPointer<float>(model, primitive, "NORMAL");
        if (hasTangent)
            T = getAttribPointer<float>(model, primitive, "TANGENT");
        if (hasUV0)
            uv0 = getAttribPointer<float>(model, primitive, "TEXCOORD_0"); // houdini attribute name: uv
        if (hasUV1)
            uv1 = getAttribPointer<float>(model, primitive, "TEXCOORD_1"); // Houdini attribute name: uv2
        if (hasCd0)
            Cd = getAttribPointer<float>(model, primitive, "COLOR_0");

        std::vector<uint32_t> prim_indices;
        const auto &indexAccessor = model.accessors[primitive.indices];
        const auto &indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const auto &indexBuffer = model.buffers[indexBufferView.buffer];

        prim_indices.resize(indexAccessor.count);
        switch (indexAccessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                const auto *indexData = reinterpret_cast<const uint16_t *>(&indexBuffer.data[
                    indexBufferView.byteOffset + indexAccessor.byteOffset]);
                for (size_t i = 0; i < indexAccessor.count; ++i) {
                    //std::cout << static_cast<uint32_t>(indexData[i]) << " ";
                    prim_indices[i] = indexData[i];
                }
                //std::cout << "\n";
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                const auto *indexData = reinterpret_cast<const uint32_t *>(&indexBuffer.data[
                    indexBufferView.byteOffset + indexAccessor.byteOffset]);
                for (size_t i = 0; i < indexAccessor.count; ++i) {
                    //std::cout << static_cast<uint32_t>(indexData[i]) << " ";.
                    prim_indices[i] = indexData[i];
                }
                //std::cout << "\n";
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                const auto *indexData = reinterpret_cast<const uint8_t *>(&indexBuffer.data[
                    indexBufferView.byteOffset + indexAccessor.byteOffset]);
                for (size_t i = 0; i < indexAccessor.count; ++i) {
                    std::cout << static_cast<uint32_t>(indexData[i]) << " ";
                    prim_indices[i] = indexData[i];
                }
                std::cout << "\n";
                break;
            }
            default: {
                std::cout << "Unsupported index component type" << std::endl;
                break;
            }
        } // end swith index component type

        for (auto index: prim_indices) {
            GLTFVertex vertex;
            vertex.P[0] = positions[index * 3 + 0];
            vertex.P[1] = positions[index * 3 + 1];
            vertex.P[2] = positions[index * 3 + 2];
            if (hasNormal) {
                vertex.N[0] = N[index * 3 + 0];
                vertex.N[1] = N[index * 3 + 1];
                vertex.N[2] = N[index * 3 + 2];
            }
            if (hasTangent) { // * 4 was found by houdini. tangent is float4...... in gltf
                vertex.T[0] = T[index * 4 + 0];
                vertex.T[1] = T[index * 4 + 1];
                vertex.T[2] = T[index * 4 + 2];
                //vertex.T = vertex.T;
                vertex.B = glm::cross(vertex.N, vertex.T);

            }
            if (hasCd0) {
                vertex.Cd[0] = Cd[index * 3 + 0];
                vertex.Cd[1] = Cd[index * 3 + 1];
                vertex.Cd[2] = Cd[index * 3 + 2];
            }
            if (hasUV0) {
                vertex.uv0[0] = uv0[index * 2 + 0];
                vertex.uv0[1] = uv0[index * 2 + 1];
            }
            if (hasUV1) {
                vertex.uv1[0] = uv1[index * 2 + 0];
                vertex.uv1[1] = uv1[index * 2 + 1];
            }
            // 查找或插入唯一顶点
            if (part.uniqueVertices.count(vertex) == 0) {
                part.uniqueVertices[vertex] = static_cast<unsigned int>(part.vertices.size());
                part.vertices.push_back(vertex);
            }
            part.indices.push_back(part.uniqueVertices[vertex]);
        }
    } // end of primitive

    for(int i=1; i<parts.size();i++) {
        parts[i].firstIndex = parts[i-1].indices.size();
    }

}




void CombinedGLTFPart::computeCombinedData() {

    /*
    // Part 1
    vertices = {v0, v1, v2, v3}
    indices = {0, 1, 2, 2, 1, 3}  // 索引值范围 0-3
    // Part 2
    vertices = {v0, v1, v2, v3}
    indices = {0, 1, 2, 2, 1, 3}  // 索引值范围也是 0-3

    // 合并的vertices = {v0, v1, v2, v3, v4, v5, v6, v7}
    //                  Part1的顶点    Part2的顶点

    // 合并的indices = {0, 1, 2, 2, 1, 3,    4, 5, 6, 6, 5, 7}
    //                 Part1的索引(0-3)       Part2的索引(4-7)
    */

    for (const auto* pPart : parts) {
        totalVertexCount += pPart->vertices.size();
        totalIndexCount += pPart->indices.size();
    }

    // 3. 合并数据并调整索引
    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;

    for (auto* pPart : parts) {
        auto &part = *pPart;
        // 添加顶点
        vertices.insert(
            vertices.end(),
            part.vertices.begin(),
            part.vertices.end()
        );

        // 复制并调整索引
        for (uint32_t index : part.indices) {
            indices.push_back(index + vertexOffset);
        }
        // 记录这个part的firstIndex
        part.firstIndex = indexOffset;
        // 更新偏移
        vertexOffset += part.vertices.size();
        indexOffset += part.indices.size();
    }
}





LLVK_NAMESPACE_END