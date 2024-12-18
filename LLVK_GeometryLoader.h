//
// Created by liuya on 8/3/2024.
//
#pragma once


#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include <array>
#include <string>
#include <unordered_map>
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
    static VkVertexInputBindingDescription bindings();
    static std::array<VkVertexInputAttributeDescription,7> attribs();
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
        uint32_t firstIndex{0}; // usefully for vkCmdDrawIndexed VkDrawIndexedIndirectCommand ...
        std::vector<GLTFVertex> vertices;
        std::vector<unsigned int> indices;
        std::unordered_map<GLTFVertex, uint32_t> uniqueVertices;
        VkBuffer verticesBuffer;
        VkBuffer indicesBuffer;
    };

    using part_t = Part;
    using vertex_t = GLTFVertex;

    void load(const std::string &path);
    std::vector<Part> parts;
    std::string filePath;
};


struct CombinedGLTFPart {
    std::vector<GLTFLoader::Part*> parts{};
    void computeCombinedData();

    std::vector<GLTFVertex> vertices{};
    std::vector<uint32_t> indices{};
    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;
};

LLVK_NAMESPACE_END
