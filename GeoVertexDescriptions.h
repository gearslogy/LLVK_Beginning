//
// Created by star on 5/9/2024.
//

#ifndef GEOVERTEXDESCRIPTIONS_H
#define GEOVERTEXDESCRIPTIONS_H
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <array>
struct Vertex{
    glm::vec3 pos;
    glm::vec3 cd;

    // for vulkan
    static VkVertexInputBindingDescription bindings();
    static std::array<VkVertexInputAttributeDescription,2> attribs();
};




#endif //GEOVERTEXDESCRIPTIONS_H
