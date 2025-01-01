//
// Created by liuya on 12/25/2024.
//
#pragma once
#include <LLVK_SYS.hpp>
#include <glm/glm.hpp>

LLVK_NAMESPACE_BEGIN
struct VTXFmt_P{
    glm::vec3 P{};      // 0
    bool operator==(const VTXFmt_P& other) const {
        return P == other.P;
    }
};
struct VTXFmt_P_N{
    glm::vec3 P{};      // 0
    glm::vec3 N{};
    bool operator==(const VTXFmt_P_N& other) const {
        return P == other.P &&
          N == other.N;
    }
};

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
    // for cubemap
    template<> struct hash<LLVK::VTXFmt_P> {
        size_t operator()(LLVK::VTXFmt_P const& vertex) const noexcept {
            return hash<glm::vec3>()(vertex.P) ;
        }
    };
    template<> struct hash<LLVK::VTXFmt_P_N> {
        size_t operator()(LLVK::VTXFmt_P_N const& vertex) const noexcept {
            return hash<glm::vec3>()(vertex.P);
        }
    };
    // for fracture
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
    struct CustomAttribLoader<VTXFmt_P_N> {
        using vertex_t = VTXFmt_P_N;
    private:
        const float *NAttribData{nullptr};
    public:
        void getAttribPointer(const tinygltf::Model &model, const tinygltf::Primitive &prim) {
            NAttribData = getRawAttribPointer<float>(model, prim, "NORMAL");
        };
        void setVertexAttrib(vertex_t & vertex, auto index) {
            vertex.N[0] = NAttribData[index * 3 + 0];
            vertex.N[1] = NAttribData[index * 3 + 1];
            vertex.N[2] = NAttribData[index * 3 + 2];
        }
    };


    // user interface for partial specialization
    template<>
    struct CustomAttribLoader<GLTFVertexVATFracture> {
        using vertex_t = GLTFVertexVATFracture;
    private:
        const uint32_t *fractureIndexAttribData{nullptr};
        const float *CdAttribData{nullptr};
        const float *NAttribData{nullptr};
        const float *TAttribData{nullptr};
        const float *uv0AttribData{nullptr};
    public:
        void getAttribPointer(const tinygltf::Model &model, const tinygltf::Primitive &prim) {
            //if (isExistAttrib(model, prim, "_fracture_index")) {}
            fractureIndexAttribData = GLTFLoaderV2::getRawAttribPointer<uint32_t>(model, prim, "_fracture_index");
            CdAttribData = GLTFLoaderV2::getRawAttribPointer<float>(model, prim, "COLOR_0");
            NAttribData = getRawAttribPointer<float>(model, prim, "NORMAL");
            TAttribData = getRawAttribPointer<float>(model, prim, "TANGENT");
            uv0AttribData = getRawAttribPointer<float>(model, prim, "TEXCOORD_0"); // houdini attribute name: uv
        };
        void setVertexAttrib(vertex_t & vertex, auto index) {
            vertex.fractureIndex = fractureIndexAttribData[index];
            vertex.Cd[0] = CdAttribData[index * 3 + 0];
            vertex.Cd[1] = CdAttribData[index * 3 + 1];
            vertex.Cd[2] = CdAttribData[index * 3 + 2];
            vertex.N[0] = NAttribData[index * 3 + 0];
            vertex.N[1] = NAttribData[index * 3 + 1];
            vertex.N[2] = NAttribData[index * 3 + 2];
            vertex.T[0] = TAttribData[index * 4 + 0];       // * 4 was found by houdini. tangent is float4...... in gltf
            vertex.T[1] = TAttribData[index * 4 + 1];
            vertex.T[2] = TAttribData[index * 4 + 2];
            vertex.uv0[0] = uv0AttribData[index * 2 + 0];
            vertex.uv0[1] = uv0AttribData[index * 2 + 1];
        }
    };
}




LLVK_NAMESPACE_END


