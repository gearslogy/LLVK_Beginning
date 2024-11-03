//
// Created by liuya on 10/19/2024.
//

#include "gltf_dump.h"


LLVK_NAMESPACE_BEGIN
struct GLTFLoader {
    // every geometry has multi parts(at least one part) that has multi materials
    struct Part {
        std::vector<GLTFVertex> vertices;
        std::vector<unsigned int> indices;
        std::unordered_map<GLTFVertex, uint32_t> uniqueVertices;
        VkBuffer verticesBuffer{};
        VkBuffer indicesBuffer{};
    };
    template<typename T>
    auto getAttribPointer(auto &model, const auto &primitive, const std::string &attribName) {
        const tinygltf::Accessor &accessor = model.accessors[primitive.attributes.find(attribName)->second];
        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
        return reinterpret_cast<const T *>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
    }
    void load(const std::string &path);
    std::vector<Part> parts;
    std::string filePath;

};

void GLTFLoader::load(const std::string &path) {
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
    filePath = path;
    std::cout << "[[GLTF model meshes size]]:" << model.meshes.size() << std::endl;
    for (const auto &mesh: model.meshes) {
        parts.resize(mesh.primitives.size());
        for (auto &&[part_id,primitive]: UT_Fn::enumerate(mesh.primitives)) {
            // if this object has two materials, there are two primitives.
            auto &part = parts[part_id];
            std::cout << "-------------primitive part:" << primitive.indices <<"-----------"<< std::endl; //
            const float *positions = getAttribPointer<float>(model, primitive, "POSITION");

            for(auto &[key, value]: primitive.attributes) {
                std::cout << "attrib key:" << key << " value:"<< value << std::endl;
            }

            bool hasNormal = (primitive.attributes.find("NORMAL") != primitive.attributes.end());
            bool hasTangent = (primitive.attributes.find("TANGENT") != primitive.attributes.end());
            bool hasUV0 = (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end());
            bool hasUV1 = (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end());
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
                uv0 = getAttribPointer<float>(model, primitive, "TEXCOORD_0");
            if (hasUV1)
                uv1 = getAttribPointer<float>(model, primitive, "TEXCOORD_1");
            if (hasCd0)
                Cd = getAttribPointer<float>(model, primitive, "COLOR_0");

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
                        prim_indices[i] = indexData[i];
                    }
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                    const auto *indexData = reinterpret_cast<const uint32_t *>(&indexBuffer.data[
                        indexBufferView.byteOffset + indexAccessor.byteOffset]);
                    for (size_t i = 0; i < indexAccessor.count; ++i) {
                        prim_indices[i] = indexData[i];
                    }
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                    const auto *indexData = reinterpret_cast<const uint8_t *>(&indexBuffer.data[
                        indexBufferView.byteOffset + indexAccessor.byteOffset]);
                    for (size_t i = 0; i < indexAccessor.count; ++i) {
                        prim_indices[i] = indexData[i];
                    }
                    break;
                }
                default: {
                    std::cout << "Unsupported index component type" << std::endl;
                    break;
                }
            } // end swith index component type
            std::cout << "gltf part indices size: " << prim_indices.size() << std::endl;
            for (const auto &index: prim_indices) {
                std::cout << index << " ";
            }
            std::cout << std::endl;

            for (auto index: prim_indices) {
                GLTFVertex vertex;
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
                if (hasUV1) {
                    vertex.uv1[0] = uv1[index * 2 + 0];
                    vertex.uv1[1] = uv1[index * 2 + 1];
                }
                // 查找或插入唯一顶点
                if (part.uniqueVertices.count(vertex) == 0) {
                    part.uniqueVertices[vertex] = static_cast<unsigned int>(part.vertices.size());
                    part.vertices.push_back(vertex);
                }
                part.indices.push_back(part.uniqueVertices[vertex]);
            }
        } // end of primitive
    } // end of modelmesh

    std::cout << "\n[[DUMP Rebuild parts size]]:" << parts.size() << std::endl;
    for(auto &&[k,v] : std::views::enumerate(parts)) {
        std::cout << "    --part:"<< k << " vertices length:" << std::size(v.vertices) << std::endl;
        std::cout << "    --part:"<< k << " indices length:" << std::size(v.indices) << std::endl;
        std::cout << "    --index :" << std::endl;
        for(auto index: v.indices) {
            std::cout << "    " <<index  << " ";
        }
        std::cout << "\n";
    }
}
LLVK_NAMESPACE_END


int main(int argc, char **argv) {
    if(argc < 2) {
        std::cout << "Error, Usage: " << argv[0] << " <gltf_file>" << std::endl;
        return -1;
    }
    const char *file = argv[1];
    LLVK::GLTFLoader loader;
    loader.load(file);
}