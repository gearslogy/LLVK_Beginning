//
// Created by liuya on 10/19/2024.
//

#ifndef GLTF_DUMP_H
#define GLTF_DUMP_H

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE


#include <libs/tiny_gltf.h>
#include "LLVK_SYS.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>


#include <iostream>
#include "Utils.h"
#include <glm/gtx/hash.hpp>



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
};
LLVK_NAMESPACE_END
namespace std {
    template<> struct hash<LLVK::GLTFVertex> {
        size_t operator()(LLVK::GLTFVertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.P) ^ (hash<glm::vec3>()(vertex.Cd) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv0) << 1);
        }
    };
};




#endif //GLTF_DUMP_H
