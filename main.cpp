#include <iostream>
#include <GLFW/glfw3.h>
#include "VulkanRenderer.h"
#include "renderer/BasicRenderer.h"
#include "renderer/DynamicsUBO/dynamicsUBO.h"
#include "renderer/vma/VmaRenderer.h"
#include "renderer/ktx_texture/ktx_texture.h"
#include "renderer/ktx_texarray/ktx_tex2darray.h"
#include "renderer/deferred/deferred.h"
#include "renderer/shadowmap/shadowmap.h"
#include "renderer/shadowmap_v2/Shadowmap_v2.h"
#include "renderer/instance/instance_v1/InstanceRenderer.h"
#include "renderer/instance/instance_v2/InstanceRendererV2.h"
#include "renderer/indirectdraw/IndirectDrawRenderer.h"
#include "renderer/dualpass/ScenePass.h"
#include "renderer/dualpass/DualPassRenderer.h"
#include "renderer/csm/CSMRenderer.h"
#include "renderer/rbd_vat/RbdVatRenderer.h"
#include "renderer/rbd_vat_storage/RbdVatStorageBufferRenderer.h"
#include "renderer/cubemap/CubeMapRenderer.h"
#include "renderer/spheremap/SphereMapRenderer.h"
#include "renderer/screenshot/ScreenShotRenderer.h"
#include "renderer/heightblend/HeightBlendRenderer.h"
int main() {
    //BasicRenderer app;
    //LLVK::ktx_texture app;
    //LLVK::DynamicsUBO app;
    //LLVK::ktx_tex2darray app;
    //LLVK::defer app;
    //LLVK::shadowmap app;
    //LLVK::Shadowmap_v2 app;
    //LLVK::InstanceRenderer app;
    //LLVK::InstanceRendererV2 app;
    //LLVK::IndirectRenderer app;
    //LLVK::DualPassRenderer app;
    //LLVK::CSMRenderer app;
    //LLVK::RbdVatRenderer app;
    //LLVK::RbdVatStorageBufferRenderer app;
    //LLVK::CUBEMAP_NAMESPACE::CubeMapRenderer app;
    //LLVK::SPHEREMAP_NAMESPACE::SphereMapRenderer app;
    //LLVK::ScreenShotRenderer app;
    LLVK::HeightBlendRenderer app;
    try {
        app.run();
    }
    catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
