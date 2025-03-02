//
// Created by liuyangping on 2025/2/26.
//

#include "SubPassResource.h"

#include <filesystem>
#include <LLVK_UT_VmaBuffer.hpp>
#include "SubPassRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
void SubPassResource::prepare() {
    assert(renderer!=nullptr);
    namespace fs = std::filesystem;
    constexpr fs::path ROOT = "content/scene/subpass";
    constexpr fs::path gltfRoot = ROOT/"gltf";
    constexpr fs::path texRoot = ROOT/"mtextures";

    const auto &device = renderer->getMainDevice().logicalDevice;
    const auto &phyDevice = renderer->getMainDevice().physicalDevice;
    // 1:Geo
    setRequiredObjectsByRenderer(renderer, geomManager);
    GLTFLoaderV2::CustomAttribLoader<Geometry::vertex_t> geoAttribSet;

    book.geoLoader.load(gltfRoot/"books.gltf", geoAttribSet);
    wall.geoLoader.load(gltfRoot/"wall.gltf", geoAttribSet);
    television.geoLoader.load(gltfRoot/"television.gltf", geoAttribSet);
    table.geoLoader.load(gltfRoot/"table.gltf", geoAttribSet);
    bottle.geoLoader.load(gltfRoot/"bottle.gltf", geoAttribSet);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(book.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(wall.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(television.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(table.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(bottle.geoLoader,geomManager);
    // 2: tex
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    setRequiredObjectsByRenderer(renderer, book.diff, book.nrm);
    setRequiredObjectsByRenderer(renderer, wall.diff, wall.nrm);
    setRequiredObjectsByRenderer(renderer, television.diff, television.nrm);
    setRequiredObjectsByRenderer(renderer, table.diff, table.nrm);
    setRequiredObjectsByRenderer(renderer, bottle.diff, bottle.nrm);

    book.diff.create(texRoot/"book/diff.png", colorSampler);
    book.nrm.create(texRoot/"book/nrm.png", colorSampler);
    wall.diff.create(texRoot/"wall/diff.png", colorSampler);
    wall.nrm.create(texRoot/"wall/nrm.png", colorSampler);
    television.diff.create(texRoot/"television/diff.png", colorSampler);
    television.nrm.create(texRoot/"television/nrm.png", colorSampler);
    table.diff.create(texRoot/"table/diff.png", colorSampler);
    table.nrm.create(texRoot/"table/nrm.png", colorSampler);
    bottle.diff.create(texRoot/"bottle/diff.png", colorSampler);
    bottle.nrm.create(texRoot/"bottle/nrm.png", colorSampler);
}

void SubPassResource::cleanup() {
    const auto &device = renderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geomManager);
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_resources(book,wall,television,table,bottle);
}



LLVK_NAMESPACE_END