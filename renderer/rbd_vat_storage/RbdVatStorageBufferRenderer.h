//
// Created by liuya on 12/24/2024.
//

#ifndef RBDVATSTORAGEBUFFERRENDERER_H
#define RBDVATSTORAGEBUFFERRENDERER_H

#include <LLVK_UT_Pipeline.hpp>
#include <utility>
#include "LLVK_SYS.hpp"
#include "VulkanRenderer.h"
#include "LLVK_ExrImage.h"
#include "LLVK_GeometryLoader.h"
#include "LLVK_GeometryLoaderV2.hpp"

LLVK_NAMESPACE_BEGIN

class RbdVatStorageBufferRenderer:public VulkanRenderer {
protected:
    void cleanupObjects() override;
    void prepare() override;
    void render() override;
private:
    static constexpr int numFrames = 128;


    struct {
        std::vector<glm::vec4> positions; // size : numRBDPacks * numFrames
        std::vector<glm::vec4> orientations; // size : numRBDPacks * numFrames
        uint32_t numRBDPacks;
    }VatData{};


    void parseStorageData();
};

LLVK_NAMESPACE_END

#endif //RBDVATSTORAGEBUFFERRENDERER_H
