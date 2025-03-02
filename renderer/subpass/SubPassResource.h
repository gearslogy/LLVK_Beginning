//
// Created by liuyangping on 2025/2/26.
//

#pragma once

#include "LLVK_SYS.hpp"
#include "LLVK_GeometryLoaderV2.hpp"
#include "LLVK_VmaBuffer.h"
#include "renderer/public/CustomVertexFormat.hpp"
LLVK_NAMESPACE_BEGIN


class SubPassRenderer;
struct SubPassResource {
    struct Geometry{
        // Geo
        using vertex_t = GLTFVertexVATFracture;
        GLTFLoaderV2::Loader<vertex_t> geoLoader;
        // Tex
        VmaUBOTexture diff;
        VmaUBOTexture nrm; // normal rough metallic
    };

    void prepare();
    void cleanup();
    SubPassRenderer *renderer{};

    Geometry book{};
    Geometry wall{};
    Geometry television{};
    Geometry table{};
    Geometry bottle{};
    VmaSimpleGeometryBufferManager geomManager{};
    VkSampler colorSampler{};

};

LLVK_NAMESPACE_END

