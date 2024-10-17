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
#include "renderer/instance/InstanceRenderer.h"
int main() {
    //BasicRenderer app;
    //LLVK::ktx_texture app;
    //LLVK::DynamicsUBO app;
    //LLVK::ktx_tex2darray app;
    //LLVK::defer app;
    //LLVK::shadowmap app;
    //LLVK::Shadowmap_v2 app;
    LLVK::InstanceRenderer app;
    try {
        app.run();
    }
    catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
