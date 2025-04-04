//
// Created by liuyangping on 2025/2/26.
//

#include "SubPassResource.h"

#include <filesystem>
#include <LLVK_UT_VmaBuffer.hpp>
#include <libs/json.hpp>
#include "SubPassRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "LLVK_UT_Json.hpp"
#include <print>
#include <glm/gtx/string_cast.hpp>
LLVK_NAMESPACE_BEGIN
void SubPassResource::prepare() {
    assert(renderer!=nullptr);
    namespace fs = std::filesystem;
    fs::path ROOT = "content/scene/subpass";
    fs::path gltfRoot = ROOT/"gltf";
    fs::path texRoot = ROOT/"mtextures";

    const auto &device = renderer->getMainDevice().logicalDevice;
    const auto &phyDevice = renderer->getMainDevice().physicalDevice;
    // 1:Geo
    setRequiredObjectsByRenderer(renderer, geomManager);
    GLTFLoaderV2::CustomAttribLoader<Geometry::vertex_t> geoAttribSet;
    //content/scene/subpass/gltf/*.gltf
    book.geoLoader.load(gltfRoot/"books.gltf", geoAttribSet);
    wall.geoLoader.load(gltfRoot/"wall.gltf", geoAttribSet);
    television.geoLoader.load(gltfRoot/"television.gltf", geoAttribSet);
    table.geoLoader.load(gltfRoot/"woodenTable.gltf", geoAttribSet);
    fabric.geoLoader.load(gltfRoot/"bottle.gltf", geoAttribSet);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(book.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(wall.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(television.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(table.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(fabric.geoLoader,geomManager);
    // 2: tex
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    setRequiredObjectsByRenderer(renderer, book.diff, book.nrm);
    setRequiredObjectsByRenderer(renderer, wall.diff, wall.nrm);
    setRequiredObjectsByRenderer(renderer, television.diff, television.nrm);
    setRequiredObjectsByRenderer(renderer, table.diff, table.nrm);
    setRequiredObjectsByRenderer(renderer, fabric.diff, fabric.nrm);
    std::println("start reading texture");
    book.diff.create(texRoot/"book/diff.png", colorSampler);
    book.nrm.create(texRoot/"book/nrm.png", colorSampler,VK_FORMAT_R8G8B8A8_SRGB);
    wall.diff.create(texRoot/"wall/diff.jpg", colorSampler);
    wall.nrm.create(texRoot/"wall/nrm.png", colorSampler,VK_FORMAT_R8G8B8A8_SRGB);
    television.diff.create(texRoot/"television/diff.jpg", colorSampler);
    television.nrm.create(texRoot/"television/nrm.png", colorSampler, VK_FORMAT_R8G8B8A8_SRGB);
    table.diff.create(texRoot/"table/diff.jpg", colorSampler);
    table.nrm.create(texRoot/"table/nrm.png", colorSampler,VK_FORMAT_R8G8B8A8_SRGB);
    fabric.diff.create(texRoot/"fabric/diff.png", colorSampler);
    fabric.nrm.create(texRoot/"fabric/nrm.png", colorSampler);

    // parse push constant data
    std::println("prepare texture end");
    prepareXform();

}
void SubPassResource::prepareXform() {
    using json = nlohmann::json;
    json jsHandle;
    std::string_view path = "content/scene/subpass/gltf/scene.json";
    std::ifstream in(path.data());
    if (!in.good())
        throw std::runtime_error{std::string{"Could not open file "} + path.data() + "."};
    in >> jsHandle;
    std::cout << "reading json file end\n";

    const auto &jsData = jsHandle["scene_nodes"].get_ref<const nlohmann::json::array_t&>();

    /*
        {
            "scene_nodes:"[{t,r,s,name},{...},{}]
        }
    */
    auto setModelMatrix = [](Geometry &obj, const glm::mat4 &model) {
        obj.xform.model = model;
        obj.xform.preModel = model;
    };
    for (auto &elem : jsData) {
        glm::vec3 s = elem["s"];
        glm::vec3 r = elem["r"];
        r = glm::radians(r);
        glm::vec3 t = elem["t"];
        std::string name = elem["name"];
        auto xform = glm::identity<glm::mat4>();
        auto xfs = glm::scale(xform, s);
        auto xfr_x = glm::rotate(xform, r.x, glm::vec3(1.0f, 0.0f, 0.0f));
        auto xfr_y = glm::rotate(xform, r.y, glm::vec3(0.0f, 1.0f, 0.0f));
        auto xfr_z = glm::rotate(xform, r.z, glm::vec3(0.0f, 0.0f, 1.0f));
        auto xt = glm::translate(glm::mat4{1.0f}, t);
        auto result = xt * xfr_z* xfr_y * xfr_x * xfs; // SRT order
        //auto result = xt * xfs;
        if (name == "books") {
            setModelMatrix(book, result);
        }

        else if (name == "wall") setModelMatrix(wall, result);
        else if (name == "television") {
            std::cout << "television trans:" << glm::to_string(t) << std::endl;
            setModelMatrix(television, result);
        }
        else if (name == "woodenTable") setModelMatrix(table, result);
        else if (name == "bottle") setModelMatrix(fabric, result);
        else {
            std::println("unrecognized name{}", name);
         }
    }
}

void SubPassResource::cleanup() {
    const auto &device = renderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geomManager);
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_resources(book,wall,television,table,fabric);
}

LLVK_NAMESPACE_END