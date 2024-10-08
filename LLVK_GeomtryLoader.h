//
// Created by liuya on 8/3/2024.
//
#pragma once


#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include <array>
#include <unordered_map>
#include <libs/tiny_gltf.h>
#include "LLVK_SYS.hpp"

LLVK_NAMESPACE_BEGIN
struct GLTFVertex{
    glm::vec3 P{};      // 0
    glm::vec3 Cd{};     // 1
    glm::vec3 N{};      // 2
    glm::vec3 T{};      // 3
    glm::vec3 B{};      // 4
    glm::vec2 uv0{};    // 5
    glm::vec2 uv1{};    // 6
    bool operator==(const GLTFVertex& other) const {
        return P == other.P && Cd== other.Cd && uv0 == other.uv0;
    }


    static constexpr int vertex_buffer_binding_id = 0;   // basic    buffer id should be at 0
    static constexpr int instance_buffer_binding_id = 1; // instance buffer id should be at 1
    // for rendering location descriptions
    // basic
    static VkVertexInputBindingDescription bindings();
    static std::array<VkVertexInputAttributeDescription,7> attribs();
    // instance
};
LLVK_NAMESPACE_END

namespace std {
    template<> struct hash<LLVK::GLTFVertex> {
        size_t operator()(LLVK::GLTFVertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.P) ^ (hash<glm::vec3>()(vertex.Cd) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv0) << 1);
        }
    };
}


LLVK_NAMESPACE_BEGIN
struct GLTFLoader {
    // every geometry has multi parts(at least one part) that has multi materials
    struct Part {
        std::vector<GLTFVertex> vertices;
        std::vector<unsigned int> indices;
        std::unordered_map<GLTFVertex, uint32_t> uniqueVertices;
        VkBuffer verticesBuffer;
        VkBuffer indicesBuffer;
    };
    template<typename T>
    auto getAttribPointer(auto &model, const auto &primitive, const std::string &attribName) {
        const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.find(attribName)->second];
        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
        return reinterpret_cast<const T *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
    }
    void load(const std::string &path);
    std::vector<Part> parts;
    std::string filePath;

    /*
    struct RenderGLTFRequiredObjects {
        const GLTFLoader *geo;
        uint32_t part;
        VkPipelineLayout layout;
        VkCommandBuffer commandBuffer;
    };


    inline void render() {
        const auto &cmdBuf = requiredData.commandBuffer;
        const auto &geo = requiredData.geo;
        //const auto &partVerticesBuffer =
        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &gridGeo.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,gridGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        //vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipelineLayout, 0, 1, &sceneSets.opaque, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, gridGeo.parts[0].indices.size(), 1, 0, 0, 0);
    }*/


};


LLVK_NAMESPACE_END
