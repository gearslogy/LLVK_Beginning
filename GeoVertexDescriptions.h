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
struct Vertex{
    glm::vec3 P{};
    glm::vec3 Cd{};
    glm::vec3 N{};
    glm::vec2 uv{};
    // for vulkan
    static VkVertexInputBindingDescription bindings();
    // P Cd N uv
    static std::array<VkVertexInputAttributeDescription,4> attribs();
    bool operator==(const Vertex& other) const {
        return P == other.P && Cd== other.Cd && uv == other.uv;
    }
};



struct ObjLoader {
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

inline void renderGeometryCommand(auto && geometry) {
    static_assert(requires {geometry.vertices;});
    static_assert(requires {geometry.indices;});
}


#endif //GEOVERTEXDESCRIPTIONS_H
