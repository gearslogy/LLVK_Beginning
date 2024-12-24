//
// Created by liuya on 12/24/2024.
//

#include "RbdVatStorageBufferRenderer.h"
#include "libs/json.hpp"
#include "LLVK_UT_Json.hpp"
LLVK_NAMESPACE_BEGIN

void RbdVatStorageBufferRenderer::cleanupObjects() {

}

void RbdVatStorageBufferRenderer::prepare() {
}

void RbdVatStorageBufferRenderer::render(){

}

void RbdVatStorageBufferRenderer::parseStorageData() {
    using json = nlohmann::json;
    json jsHandle;
    std::string_view path = "content/scene/storage_rbdvat/scene.json";
    std::ifstream in(path.data());
    if (!in.good())
        throw std::runtime_error{std::string{"Could not open file "} + path.data() + "."};
    in >> jsHandle;
    if(not jsHandle.contains("points") )
        throw std::runtime_error{std::string{"Could not parse JSON file, no points key "} };
    const nlohmann::json &points = jsHandle["points"];

    for (auto &data: points) {
        const auto &P = data["P"].get<glm::vec3>();
        const auto &orient = data["orient"].get<glm::vec4>();
    }
    //std::cout << "[[JsonSceneParser]]" << " npts:" << instanceData.size() << std::endl;
}


LLVK_NAMESPACE_END