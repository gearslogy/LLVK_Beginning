//
// Created by star on 5/9/2024.
//

#ifndef GEOVERTEXDESCRIPTIONS_H
#define GEOVERTEXDESCRIPTIONS_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <unordered_map>


struct Instance {
    glm::vec3 P;
    glm::vec3 scale;
    glm::vec3 rotation;
    uint32_t texIndex;
};

struct Vertex{
    glm::vec3 P{};
    glm::vec3 Cd{};
    glm::vec3 N{};
    glm::vec2 uv{};


    bool operator==(const Vertex& other) const {
        return P == other.P && Cd== other.Cd && uv == other.uv;
    }

    // basic
    static VkVertexInputBindingDescription bindings();
    static std::array<VkVertexInputAttributeDescription,4> attribs();    // P Cd N uv
    // INSTANCE
    static std::array<VkVertexInputBindingDescription,2> instancedBindings();
    static std::array<VkVertexInputAttributeDescription,8> instancedAttribs(); // P Cd N uv I.P I.Scale I.rot I.texIdx

    static constexpr int vertex_buffer_binding_id = 0;   // basic    buffer id should be at 0
    static constexpr int instance_buffer_binding_id = 1; // instance buffer id should be at 1


};



struct IndicesGeometryBuffer {
    VkBuffer verticesBuffer;
    VkBuffer indicesBuffer;
};

struct Quad : IndicesGeometryBuffer{
    //  (Vulkan NDC coordinates, CCW)
    constexpr inline void init() {
        vertices[0].P = {-1,1,0}; // bottom left
        vertices[1].P = {1,1,0}; // bottom right
        vertices[2].P = {1,-1,0}; // top right
        vertices[3].P = {-1,-1,0}; // top left

        vertices[0].uv = {0,1};
        vertices[1].uv = {1,1};
        vertices[2].uv = {1,0};
        vertices[3].uv = {0,0};
        indices = {0,1,2,2,3,0}; // CCW
    }
    std::array<Vertex,4> vertices{};
    std::array<uint32_t,6> indices{};
};


struct ObjLoader:IndicesGeometryBuffer {
    void readFile(const char *geoPath);
    std::vector<Vertex> vertices; // REAL GEOMETR DATA
    std::vector<uint32_t> indices;
};
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.P) ^ (hash<glm::vec3>()(vertex.Cd) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}

inline void createVertexAndIndexBuffer(auto &&bufferManager, auto &geometry) {
    static_assert(requires { geometry.vertices; });
    static_assert(requires { geometry.indices; });
    static_assert(requires { bufferManager.createdBuffers; });
    static_assert(requires { bufferManager.createdIndexedBuffers; });

    bufferManager.createVertexBufferWithStagingBuffer(sizeof(Vertex) * geometry.vertices.size(),
                                                      geometry.vertices.data());
    bufferManager.createIndexBuffer(sizeof(uint32_t) * geometry.indices.size(),
                                    geometry.indices.data());

    // assgin right buffer-ptr
    auto &&verticesBuffers = bufferManager.createdBuffers; // return struct BufferAndMemory
    auto &&indicesBuffers = bufferManager.createdIndexedBuffers;
    geometry.verticesBuffer = verticesBuffers[verticesBuffers.size() - 1].buffer;
    geometry.indicesBuffer = indicesBuffers[indicesBuffers.size() - 1].buffer;
}

inline void renderIndicesGeometryCommand(auto &&geometry, VkCommandBuffer cmdBuffer,
                                         const std::function<void()> &FN_CmdBindDescriptorSets = nullptr) {
    static_assert(requires { geometry.vertices; });
    static_assert(requires { geometry.indices; });

    // only one buffer hold all data
    VkDeviceSize offset =0;
    vkCmdBindVertexBuffers(cmdBuffer,
                           0,
                           1,
                           &geometry.verticesBuffer,&offset);
    if (FN_CmdBindDescriptorSets)FN_CmdBindDescriptorSets();
    vkCmdBindIndexBuffer(cmdBuffer, geometry.indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuffer, geometry.indices.size(), 1, 0, 0, 0);
}


#endif //GEOVERTEXDESCRIPTIONS_H
