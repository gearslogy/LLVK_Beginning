//
// Created by liuyangping on 2025/2/26.
//

#pragma once

#include "LLVK_SYS.hpp"
#include "LLVK_GeometryLoaderV2.hpp"
#include "LLVK_VmaBuffer.h"
#include "SubpassTypes.hpp"
#include "renderer/public/CustomVertexFormat.hpp"
LLVK_NAMESPACE_BEGIN


class SubPassRenderer;
struct SubPassResource {
    struct Geometry{
        // Geo
        using vertex_t = VTXFmt_P_N_T_UV0;
        GLTFLoaderV2::Loader<vertex_t> geoLoader;
        // Tex
        VmaUBOTexture diff;
        VmaUBOTexture nrm; // normal rough metallic
        // xform. preXform need set in main renderer
        subpass::xform xform; // model matrix
        void cleanup() {
            diff.cleanup();
            nrm.cleanup();
        }
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
    void prepareXform();
};

LLVK_NAMESPACE_END

