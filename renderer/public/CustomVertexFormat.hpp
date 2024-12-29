//
// Created by liuya on 12/25/2024.
//
#pragma once
#include <LLVK_SYS.hpp>
#include <glm/glm.hpp>

LLVK_NAMESPACE_BEGIN
// fracture VAT vertex format
struct GLTFVertexVATFracture {
    glm::vec3 P{};      // 0
    glm::vec3 Cd{};     // 1
    glm::vec3 N{};      // 2
    glm::vec3 T{};      // 3
    glm::vec2 uv0{};    // 4
    glm::int32_t fractureIndex{}; // 5
    bool operator==(const GLTFVertexVATFracture& other) const {
        return P == other.P &&
            Cd== other.Cd &&
            uv0 == other.uv0 &&
            fractureIndex == other.fractureIndex && N == other.N ; // IMPORTANT, DO NOT COMBINE N! DON't use combined N(point)
    }
};
LLVK_NAMESPACE_END

namespace std {
    template<> struct hash<LLVK::GLTFVertexVATFracture> {
        size_t operator()(LLVK::GLTFVertexVATFracture const& vertex) const noexcept {
            return ((hash<glm::vec3>()(vertex.P) ^ (hash<glm::vec3>()(vertex.Cd) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv0) << 1);
        }
    };
}

LLVK_NAMESPACE_BEGIN
namespace GLTFLoaderV2 {
    inline constexpr auto isExistAttrib = [](auto &model, const auto &primitive, const auto &attribName) {
        return primitive.attributes.find(attribName) != primitive.attributes.end() ;
    };
    // user interface for partial specialization
    template<>
    struct CustomAttribLoader<GLTFVertexVATFracture> {
        using vertex_t = GLTFVertexVATFracture;
    private:
        const uint32_t *fractureIndexAttribData{nullptr};
        const float *CdAttribData{nullptr};
    public:
        void getAttribPointer(const tinygltf::Model &model, const tinygltf::Primitive &prim) {
            //if (isExistAttrib(model, prim, "_fracture_index")) {}
            fractureIndexAttribData = GLTFLoaderV2::getRawAttribPointer<uint32_t>(model, prim, "_fracture_index");
            CdAttribData = GLTFLoaderV2::getRawAttribPointer<float>(model, prim, "COLOR_0");
        };
        void setVertexAttrib(vertex_t & vertex, auto index) {
            vertex.fractureIndex = fractureIndexAttribData[index];
            vertex.Cd[0] = CdAttribData[index * 3 + 0];
            vertex.Cd[1] = CdAttribData[index * 3 + 1];
            vertex.Cd[2] = CdAttribData[index * 3 + 2];
        }
    };
}

LLVK_NAMESPACE_END


