//
// Created by star on 5/9/2024.
//

#include "GeoVertexDescriptions.h"
VkVertexInputBindingDescription Vertex::bindings() {
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(Vertex);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}


std::array<VkVertexInputAttributeDescription,2> Vertex::attribs() {
    std::array<VkVertexInputAttributeDescription,2> desc{};
    desc[0].binding = 0;
    desc[0].location = 0;
    desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[0].offset = offsetof(Vertex, pos);

    desc[1].binding = 0;
    desc[1].location = 1;
    desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    desc[1].offset = offsetof(Vertex, cd);
    return desc;
}

