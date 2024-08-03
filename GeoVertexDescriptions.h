//
// Created by star on 5/9/2024.
//
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <unordered_map>
#include "LLVK_SYS.hpp"

LLVK_NAMESPACE_BEGIN
namespace Basic {
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

}


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
    std::array<Basic::Vertex,4> vertices{};
    std::array<uint32_t,6> indices{};
};


struct ObjLoader:IndicesGeometryBuffer {
    void readFile(const char *geoPath);
    std::vector<Basic::Vertex> vertices; // REAL GEOMETR DATA
    std::vector<uint32_t> indices;
};
LLVK_NAMESPACE_END
namespace std {
    template<> struct hash<LLVK::Basic::Vertex> {
        size_t operator()(LLVK::Basic::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.P) ^ (hash<glm::vec3>()(vertex.Cd) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}
