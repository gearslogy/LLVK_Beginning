//
// Created by liuya on 12/13/2024.
//

#pragma once

#include "LLVK_SYS.hpp"
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>
#include <iostream>
#include "LLVK_Utils.hpp"
#include <libs/tiny_gltf.h>
LLVK_NAMESPACE_BEGIN
struct GLTFVertexVATFracture {
    glm::vec3 P{};      // 0
    glm::vec3 Cd{};     // 1
    glm::vec3 N{};      // 2
    glm::vec3 T{};      // 3
    glm::vec2 uv0{};    // 4
    glm::int32_t fractureIndex{};
};
LLVK_NAMESPACE_END


namespace std {
    template<> struct hash<LLVK::GLTFVertexVATFracture> {
        size_t operator()(LLVK::GLTFVertexVATFracture const& vertex) const {
            return ((hash<glm::vec3>()(vertex.P) ^ (hash<glm::vec3>()(vertex.Cd) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv0) << 1);
        }
    };
}


LLVK_NAMESPACE_BEGIN

namespace GLTFLoaderV2 {

    constexpr auto isExistAttrib = [](auto &model, const auto &primitive, const auto &attribName) {
        return primitive.attributes.find(attribName) != primitive.attributes.end() ;
    };
    // get geometry raw buffer
    template<typename T>
    auto getAttribPointer(auto &model, const auto &primitive, const std::string &attribName) {
        const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.find(attribName)->second];
        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
        return reinterpret_cast<const T *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
    }


    template<typename vert_t>
    struct Part {
        using vertex_t = vert_t;
        uint32_t firstIndex{0}; // usefully for vkCmdDrawIndexed VkDrawIndexedIndirectCommand ...
        std::vector<vert_t> vertices;
        std::vector<unsigned int> indices;
        std::unordered_map<vert_t, uint32_t> uniqueVertices;
        VkBuffer verticesBuffer;
        VkBuffer indicesBuffer;
    };

    template<typename vert_t, typename data_type>
    struct CustomAttribLoader;


    // user interface for partial specialization
    template<typename data_type>
    struct CustomAttribLoader<GLTFVertexVATFracture, data_type> {
        using vertex_t = GLTFVertexVATFracture;
        explicit CustomAttribLoader(const std::string &name) : attribName(name) {}
    private:
        bool exist{false};
        std::string attribName{};
        const data_type *attrib_data{nullptr};
    public:
        void getAttribPointer(const tinygltf::Model &model, const tinygltf::Primitive &prim) {
            exist = isExistAttrib(model, prim, attribName);
            if (exist)
                attrib_data = getAttribPointer(model, prim, attribName);
        };

        void setVertexAttrib(vertex_t & vertex, auto index) {
            if (exist) vertex.fractureIndex = attrib_data[index];
        }
    };




    template<typename part_t> void load(const std::string &path, std::vector<part_t> &parts, auto &&... customAttribLoader) {
        using vertex_t = typename part_t::vertex_t;
        std::cout << "[[GLTFLoader::load]]:" << path << std::endl;
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        if (!warn.empty()) {
            std::cerr << "Warning: " << warn << std::endl;
        }
        if (!err.empty()) {
            std::cerr << "Error: " << err << std::endl;
        }
        if (!ret) {
            std::cerr << "Failed to load GLTF file" << std::endl;
        }
        // OUR LIB ONLY SUPPORT HOUDINI shopmaterial_path. so model.meshes must be ==1
        assert(model.meshes.size()==1);
        const auto &mesh = model.meshes[0]; // I found houdini name attribute will build multi meshes. but now I just want to get single model, single mesh, multi primitives
        parts.resize(mesh.primitives.size());
        for (auto &&[part_id,primitive]: UT_Fn::enumerate(mesh.primitives)) {
            // if this object has two materials, there are two primitives.
            auto &part = parts[part_id];
            std::cout << "primitive indices:" << primitive.indices << std::endl; //
            const float *positions = getAttribPointer<float>(model, primitive, "POSITION");

            bool hasNormal = (primitive.attributes.find("NORMAL") != primitive.attributes.end());
            bool hasTangent = (primitive.attributes.find("TANGENT") != primitive.attributes.end());
            bool hasUV0 = (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end());
            //bool hasUV1 = (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end());
            bool hasCd0 = (primitive.attributes.find("COLOR_0") != primitive.attributes.end());
            // fix attribute rendering!
            const float *N = nullptr;
            const float *T = nullptr;
            const float *uv0 = nullptr;
            const float *uv1 = nullptr;
            const float *Cd = nullptr;

            if (hasNormal)
                N = getAttribPointer<float>(model, primitive, "NORMAL");
            if (hasTangent)
                T = getAttribPointer<float>(model, primitive, "TANGENT");
            if (hasUV0)
                uv0 = getAttribPointer<float>(model, primitive, "TEXCOORD_0"); // houdini attribute name: uv
            //if (hasUV1)
            //    uv1 = getAttribPointer<float>(model, primitive, "TEXCOORD_1"); // Houdini attribute name: uv2
            if (hasCd0)
                Cd = getAttribPointer<float>(model, primitive, "COLOR_0");


            auto extraAttribLoaders = std::forward_as_tuple(std::forward<decltype(customAttribLoader)>(customAttribLoader)...);
            std::apply([&](auto&... loaders) {
                (loaders.getAttribPointer(model, primitive), ...);
            }, extraAttribLoaders);


            std::vector<uint32_t> prim_indices;
            const auto &indexAccessor = model.accessors[primitive.indices];
            const auto &indexBufferView = model.bufferViews[indexAccessor.bufferView];
            const auto &indexBuffer = model.buffers[indexBufferView.buffer];

            prim_indices.resize(indexAccessor.count);
            switch (indexAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                    const auto *indexData = reinterpret_cast<const uint16_t *>(&indexBuffer.data[
                        indexBufferView.byteOffset + indexAccessor.byteOffset]);
                    for (size_t i = 0; i < indexAccessor.count; ++i) {
                        //std::cout << static_cast<uint32_t>(indexData[i]) << " ";
                        prim_indices[i] = indexData[i];
                    }
                    //std::cout << "\n";
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                    const auto *indexData = reinterpret_cast<const uint32_t *>(&indexBuffer.data[
                        indexBufferView.byteOffset + indexAccessor.byteOffset]);
                    for (size_t i = 0; i < indexAccessor.count; ++i) {
                        //std::cout << static_cast<uint32_t>(indexData[i]) << " ";.
                        prim_indices[i] = indexData[i];
                    }
                    //std::cout << "\n";
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                    const auto *indexData = reinterpret_cast<const uint8_t *>(&indexBuffer.data[
                        indexBufferView.byteOffset + indexAccessor.byteOffset]);
                    for (size_t i = 0; i < indexAccessor.count; ++i) {
                        std::cout << static_cast<uint32_t>(indexData[i]) << " ";
                        prim_indices[i] = indexData[i];
                    }
                    std::cout << "\n";
                    break;
                }
                default: {
                    std::cout << "Unsupported index component type" << std::endl;
                    break;
                }
            } // end swith index component type

            for (auto index: prim_indices) {
                vertex_t vertex;
                vertex.P[0] = positions[index * 3 + 0];
                vertex.P[1] = positions[index * 3 + 1];
                vertex.P[2] = positions[index * 3 + 2];
                if (hasNormal) {
                    vertex.N[0] = N[index * 3 + 0];
                    vertex.N[1] = N[index * 3 + 1];
                    vertex.N[2] = N[index * 3 + 2];
                }
                if (hasTangent) { // * 4 was found by houdini. tangent is float4...... in gltf
                    vertex.T[0] = T[index * 4 + 0];
                    vertex.T[1] = T[index * 4 + 1];
                    vertex.T[2] = T[index * 4 + 2];
                    //vertex.T = vertex.T;
                    vertex.B = glm::cross(vertex.N, vertex.T);
                }
                if (hasCd0) {
                    vertex.Cd[0] = Cd[index * 3 + 0];
                    vertex.Cd[1] = Cd[index * 3 + 1];
                    vertex.Cd[2] = Cd[index * 3 + 2];
                }
                if (hasUV0) {
                    vertex.uv0[0] = uv0[index * 2 + 0];
                    vertex.uv0[1] = uv0[index * 2 + 1];
                }
                //if (hasUV1) {
                    //vertex.uv1[0] = uv1[index * 2 + 0];
                    //vertex.uv1[1] = uv1[index * 2 + 1];
                //}
                std::apply([&](auto&... loaders) {
                    (loaders.setVertexAttrib(vertex, index), ...);
                }, extraAttribLoaders);

                // 查找或插入唯一顶点
                if (part.uniqueVertices.count(vertex) == 0) {
                    part.uniqueVertices[vertex] = static_cast<unsigned int>(part.vertices.size());
                    part.vertices.push_back(vertex);
                }
                part.indices.push_back(part.uniqueVertices[vertex]);
            }
        } // end of primitive

        for(int i=1; i<parts.size();i++) {
            parts[i].firstIndex = parts[i-1].indices.size();
        }

    } // end of loader function


    template<typename vertex_type>
    struct Loader{
        using vertex_t = vertex_type;
        using part_t = Part<vertex_t>;
        std::vector<part_t> parts{};
        inline void load(const std::string &path, auto && ... customAttribLoaders){ load(path, parts, customAttribLoaders...);}
    };
} // end of GLTFLoaderV2 namespace


LLVK_NAMESPACE_END

