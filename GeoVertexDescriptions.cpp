//
// Created by star on 5/9/2024.
//

#include "GeoVertexDescriptions.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "libs/tiny_obj_loader.h"
LLVK_NAMESPACE_BEGIN
namespace Basic {
VkVertexInputBindingDescription Vertex::bindings() {
    VkVertexInputBindingDescription desc{};
    desc.binding = vertex_buffer_binding_id;
    desc.stride = sizeof(Vertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}


std::array<VkVertexInputAttributeDescription,4> Vertex::attribs() {
    std::array<VkVertexInputAttributeDescription,4> desc{};
    desc[0].binding = vertex_buffer_binding_id;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(Vertex, P);

    desc[1].binding = vertex_buffer_binding_id;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(Vertex, Cd);

    desc[2].binding = vertex_buffer_binding_id;
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[2].offset = offsetof(Vertex, N);

    desc[3].binding = vertex_buffer_binding_id;
    desc[3].location = 3;
    desc[3].format = VK_FORMAT_R32G32_SFLOAT;
    desc[3].offset = offsetof(Vertex, uv);
    return desc;
}

std::array<VkVertexInputBindingDescription,2>  Vertex::instancedBindings() {
    VkVertexInputBindingDescription desc0{};
    desc0.binding = vertex_buffer_binding_id;
    desc0.stride = sizeof(Vertex);
    desc0.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputBindingDescription desc1{};
    desc1.binding = instance_buffer_binding_id;
    desc1.stride = sizeof(Instance);
    desc1.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    return {desc0,desc1};
}

std::array<VkVertexInputAttributeDescription, 8> Vertex::instancedAttribs() {
    std::array<VkVertexInputAttributeDescription,8> desc{};
    desc[0].binding = vertex_buffer_binding_id;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(Vertex, P);

    desc[1].binding = vertex_buffer_binding_id;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(Vertex, Cd);

    desc[2].binding = vertex_buffer_binding_id;
    desc[2].location = 2;
    desc[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[2].offset = offsetof(Vertex, N);

    desc[3].binding = vertex_buffer_binding_id;
    desc[3].location = 3;
    desc[3].format = VK_FORMAT_R32G32_SFLOAT;
    desc[3].offset = offsetof(Vertex, uv);

    desc[4] = { 4,instance_buffer_binding_id,VK_FORMAT_R32G32B32_SFLOAT , offsetof(Instance, P)};
    desc[5] = { 5,instance_buffer_binding_id,VK_FORMAT_R32G32B32_SFLOAT , offsetof(Instance, P)};
    desc[6] = { 6,instance_buffer_binding_id,VK_FORMAT_R32G32B32_SFLOAT , offsetof(Instance, P)};
    desc[7] = { 7,instance_buffer_binding_id,VK_FORMAT_R32_UINT , offsetof(Instance, P)};

    return desc;
}
}



void ObjLoader::readFile(const char *geoPath) {
    tinyobj::attrib_t attrib;

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, geoPath)) {
        throw std::runtime_error(err);
    }
    bool hasNormal = !attrib.normals.empty();
    bool hasST = !attrib.texcoords.empty();

    std::unordered_map<Basic::Vertex, uint32_t> uniqueVertices{};

    for (auto &shape: shapes) {
        for (auto idx: shape.mesh.indices) {
            Basic::Vertex vertex{};
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

LLVK_NAMESPACE_END
