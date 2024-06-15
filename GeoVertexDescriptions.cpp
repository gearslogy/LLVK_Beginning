//
// Created by star on 5/9/2024.
//

#include "GeoVertexDescriptions.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "libs/tiny_obj_loader.h"
VkVertexInputBindingDescription Vertex::bindings() {
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(Vertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}


std::array<VkVertexInputAttributeDescription,4> Vertex::attribs() {
    std::array<VkVertexInputAttributeDescription,4> desc{};
    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(Vertex, P);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(Vertex, Cd);

    desc[2].binding = 0;
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[2].offset = offsetof(Vertex, N);

    desc[3].binding = 0;
    desc[3].location = 3;
    desc[3].format = VK_FORMAT_R32G32_SFLOAT;
    desc[3].offset = offsetof(Vertex, uv);
    return desc;
}

void ObjLoader::readFile(const char *geoPath) {
    tinyobj::attrib_t attrib;

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, geoPath)) {
        throw std::runtime_error(err);
    }
    bool hasNormal = attrib.normals.size()!=0;
    bool hasST = attrib.texcoords.size()!=0;

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (auto &shape: shapes) {
        for (auto idx: shape.mesh.indices) {
            Vertex vertex{};
            vertex.P = {
                attrib.vertices[3 * idx.vertex_index + 0],
                attrib.vertices[3 * idx.vertex_index + 1],
                attrib.vertices[3 * idx.vertex_index + 2]
            };
            if (hasNormal) {
                vertex.N = {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };
            }

            if (hasST) {
                vertex.uv = {
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]
                };
            }
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);

        }
    }

}


