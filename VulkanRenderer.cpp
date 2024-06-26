
//
// Created by liuyangping on 2024/3/27.
//

#include "VulkanRenderer.h"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <cassert>
#include <set>
#include "VulkanValidation.h"
#include "magic_enum.hpp"
#include <format>
#include <filesystem>

#include "GeoVertexDescriptions.h"


/* DRAW TRANGLE
 static Vertex triangle[3] = {
     {{0,-0.4,0},{ 1.0,0.0,0.0}},
     {{0.4,0.4,0},{0.0,1.0,0.0}},
     {{-0.4,0.4,0},{0,0,1}},
 };
 //simpleVertexBuffer.createVertexBuffer(sizeof(Vertex) * 3, triangle);
 simpleVertexBuffer.createVertexBufferWithStagingBuffer(sizeof(Vertex) * 3, triangle);
 */

// 1.假设正常情况下直觉模型做法：Y是超上。
// DRAW QUAD WITH INDEX BUFFER.
// 注意虽然裁剪坐标Y是反得。我们得模型必须按照Y朝上，就是Houdini那样定义。所以下面得模型是这个顺序。
// 绘制顺序: CCW逆时针绘制.
// 设置：VkPipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
// projection矩阵必须ubo1.proj[1][1] *= -1; 要不图像是反得。而且看不见

/*
 static std::vector<Vertex> vertices = {
     {{-0.5f, -0.5f,0}, {1.0f, 0.0f, 0.0f}}, // 左下角
     {{0.5f, -0.5f,0}, {0.0f, 1.0f, 0.0f}},  // 右下角
     {{0.5f, 0.5f,0}, {0.0f, 0.0f, 1.0f}},   // 右上角
     {{-0.5f, 0.5f,0}, {1.0f, 1.0f, 1.0f}}   // 左上角
 };
static std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};*/
// 2. 当新手vulkan入门时候：
// 由于这几个坐标是在裁剪坐标下做得，所以意义会成这个样子：
// Y朝下：
// 绘制顺序：CW 顺时针
// 必须设置：VkPipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
/*
static std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f,0}, {1.0f, 0.0f, 0.0f}}, // 左上角
    {{0.5f, -0.5f,0}, {0.0f, 1.0f, 0.0f}},  // 右上角
    {{0.5f, 0.5f,0}, {0.0f, 0.0f, 1.0f}},   // 右下角
    {{-0.5f, 0.5f,0}, {1.0f, 1.0f, 1.0f}}   // 左下角
};
*/

// with uv
/*
static std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f,0}, {1.0f, 0.0f, 0.0f},{ 0.0f, 1.0f } }, // 左下角
    {{0.5f, -0.5f,0}, {0.0f, 1.0f, 0.0f},{ 1.0f, 1.0f }},  // 右下角
    {{0.5f, 0.5f,0}, {0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f }},   // 右上角
    {{-0.5f, 0.5f,0}, {1.0f, 1.0f, 1.0f},{ 0.0f, 0.0f }}   // 左上角
};
static std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};
*/

/*
static float offset= 0.3;
static std::vector<Vertex> vertices = {
    {{-0.5f - offset, -0.5f,0}, {1.0f, 0.0f, 0.0f},{0,1,0},{ 0.0f, 1.0f } }, // 左下角
    {{0.5f - offset, -0.5f,0}, {0.0f, 1.0f, 0.0f},{0,1,0},{ 1.0f, 1.0f }},  // 右下角
    {{0.5f - offset, 0.5f,0}, {0.0f, 0.0f, 1.0f},{0,1,0}, { 1.0f, 0.0f }},   // 右上角
    {{-0.5f - offset, 0.5f,0}, {1.0f, 1.0f, 1.0f},{0,1,0},{ 0.0f, 0.0f }},   // 左上角

    {{-0.5f + offset, -0.5f,-0.5}, {1.0f, 0.0f, 0.0f},{0,1,0},{ 0.0f, 1.0f } }, // 左下角
    {{0.5f +offset, -0.5f,-0.5}, {0.0f, 1.0f, 0.0f},{0,1,0},{ 1.0f, 1.0f }},  // 右下角
    {{0.5f +offset, 0.5f,-0.5}, {0.0f, 0.0f, 1.0f},{0,1,0}, { 1.0f, 0.0f }},   // 右上角
    {{-0.5f + offset, 0.5f,-0.5}, {1.0f, 1.0f, 1.0f},{0,1,0},{ 0.0f, 0.0f }}   // 左上角
};
static std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
*/





VulkanRenderer::VulkanRenderer() {}


void VulkanRenderer::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height ) {
        auto app = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
        std::cout << "glfw windows size changed:" << width << " " << height << std::endl;
        app->framebufferResized = true;
    });
}
void VulkanRenderer::mainLoop() {
    while (!glfwWindowShouldClose(window)){
        glfwPollEvents();
        draw();
    }
}
void VulkanRenderer::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

int VulkanRenderer::initVulkan() {
    try{
        createInstance();
        createDebugCallback();
        createSurface();
        createPhyiscalAndLogicDevice();
        createSwapChain();
        createRenderpass();
        createDescriptorSetLayout();
        createPipeline();
        createDepthResources();
        createFramebuffers();
        createCommandPool();
        createTexture();
        loadModel();
        createVertexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }
    catch (const std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
void VulkanRenderer::cleanup() {
    // Wait until no actions being run on device before destroying
    vkDeviceWaitIdle(mainDevice.logicalDevice);
    cleanupSwapChain();
    simpleDescriptorManager.cleanup();
    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        vkDestroySemaphore(mainDevice.logicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(mainDevice.logicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(mainDevice.logicalDevice,inFlightFences[i], nullptr);
    }
    simpleCommandManager.cleanup();
    simpleVertexBuffer.cleanup();
    simplePipeline.cleanup();
    simplePass.cleanup();
    vkDestroySurfaceKHR(instance,surfaceKhr, nullptr);
    mainDevice.cleanup();
    if(enableValidation){
        simpleDebug.cleanup();
    }
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}
void VulkanRenderer::cleanupSwapChain() {
    vkDestroyImage(mainDevice.logicalDevice, depthImageAndMemory.image, nullptr);
    vkDestroyImageView(mainDevice.logicalDevice, depthImageView, nullptr);
    vkFreeMemory(mainDevice.logicalDevice, depthImageAndMemory.memory, nullptr);
    simpleFramebuffer.cleanup();
    simpleSwapchain.cleanup();
}
void VulkanRenderer::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(mainDevice.logicalDevice);
    cleanupSwapChain();
    createSwapChain();
    createDepthResources();
    createFramebuffers();
}




void VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "TestEngineApp",
            .applicationVersion = VK_VERSION_1_3,
            .pEngineName = "TestEngine",
            .engineVersion = VK_VERSION_1_3,
            .apiVersion = VK_API_VERSION_1_3,
    };

    uint32_t glfwExtPropsCount;
    const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtPropsCount);
    std::vector<const char *> extensionNamesList;
    for(int i=0;i<glfwExtPropsCount;i++){
        extensionNamesList.emplace_back(glfwExtensionNames[i]);
    }
    if(enableValidation) {
        //extensionNamesList.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME); // VK_EXT_debug_report, this is old method(v1)
        extensionNamesList.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);  // we will use here
    }

    checkInstanceExtensionSupport(extensionNamesList);

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = DebugV2::CustomDebug::getCreateInfo();
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensionNamesList.size();
    instanceCreateInfo.ppEnabledExtensionNames = extensionNamesList.data();
    instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    std::vector<const char *> layerNames = {
            //"VK_LAYER_LUNARG_api_dump", // SUPPORT
            //"VK_LAYER_LUNARG_standard_validation" // NOT SUPPORTED
            "VK_LAYER_KHRONOS_validation"
    };
    if(enableValidation){
        instanceCreateInfo.ppEnabledLayerNames = layerNames.data();
        instanceCreateInfo.enabledLayerCount = layerNames.size();
    }
    // create instance
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if(result != VK_SUCCESS){
        throw std::runtime_error("Failed create vulkan instance");
    }
}

void VulkanRenderer::createPhyiscalAndLogicDevice() {
    mainDevice.bindSurface = surfaceKhr;
    mainDevice.bindInstance = instance;
    mainDevice.init();
}

void VulkanRenderer::createDebugCallback() {
    if(not enableValidation) return;
    simpleDebug.bindInstance = instance;
    simpleDebug.init();
}

void VulkanRenderer::checkInstanceExtensionSupport(const std::vector<const char *> &checkExtensions) {
    uint32_t allSupportInstanceExtensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &allSupportInstanceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> instancePropertiesList(allSupportInstanceExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &allSupportInstanceExtensionCount, instancePropertiesList.data());
    for(const auto &checkExt : checkExtensions){
        auto find = std::find_if(instancePropertiesList.begin(), instancePropertiesList.end(), [&checkExt](VkExtensionProperties &vkInstanceExtProp){
            if(strcmp(vkInstanceExtProp.extensionName , checkExt) == 0 ) return true;
            return false;
        });
        if(find!= instancePropertiesList.end()) {
            //std::cout << "vulkan support instance extension:" << checkExt << std::endl;
        }
        else{
            throw std::runtime_error(std::format("vulkan not support this instance extension:{}", checkExt));
        }
    };
}

void VulkanRenderer::createSurface() {
    auto result = glfwCreateWindowSurface(instance, window, nullptr, &surfaceKhr);
    if(result!= VK_SUCCESS) {
        throw std::runtime_error{"ERROR create surface"};
    }
}


void VulkanRenderer::createSwapChain() {
    simpleSwapchain.bindLogicDevice = mainDevice.logicalDevice;
    simpleSwapchain.bindPhyiscalDevice = mainDevice.physicalDevice;
    simpleSwapchain.bindSurface = surfaceKhr;
    simpleSwapchain.bindWindow = window;
    simpleSwapchain.init();
}
void VulkanRenderer::createDescriptorSetLayout() {
    simpleDescriptorManager.bindDevice = mainDevice.logicalDevice;
    simpleDescriptorManager.createDescriptorSetLayout();
}

void VulkanRenderer::createPipeline(){
    simplePipeline.bindDevice = mainDevice.logicalDevice;
    simplePipeline.bindExtent = simpleSwapchain.swapChainExtent;
    simplePipeline.bindRenderPass = simplePass.pass;
    simplePipeline.bindDescriptorSetLayouts[0] = simpleDescriptorManager.ubo_descriptorSetLayout;
    simplePipeline.bindDescriptorSetLayouts[1] = simpleDescriptorManager.texture_descriptorSetLayout;
    simplePipeline.init();
}
void VulkanRenderer::createDepthResources() {
    auto depthFormat = FnImage::findDepthFormat(mainDevice.physicalDevice);
    depthImageAndMemory = FnImage::createImageAndMemory(mainDevice.physicalDevice,mainDevice.logicalDevice,
        simpleSwapchain.swapChainExtent.width,
        simpleSwapchain.swapChainExtent.height,
        1,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    depthImageView = FnImage::createImageView(mainDevice.logicalDevice,
        depthImageAndMemory.image,
        depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,1);
}

void VulkanRenderer::createRenderpass(){
    simplePass.bindDevice = mainDevice.logicalDevice;
    simplePass.bindPhysicalDevice = mainDevice.physicalDevice;
    simplePass.init();
}
void VulkanRenderer::createFramebuffers() {
    simpleFramebuffer.bindDevice = mainDevice.logicalDevice;
    simpleFramebuffer.bindRenderPass = simplePass.pass;
    simpleFramebuffer.bindSwapChainExtent = &simpleSwapchain.swapChainExtent;
    simpleFramebuffer.bindSwapChainImages = &simpleSwapchain.swapChainImages;
    simpleFramebuffer.bindDepthImageView = depthImageView;
    simpleFramebuffer.init();
}
void VulkanRenderer::createCommandPool() {
    simpleCommandManager.bindLogicDevice = mainDevice.logicalDevice;
    simpleCommandManager.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleCommandManager.bindSurface = surfaceKhr;
    simpleCommandManager.createGraphicsCommandPool();
}
void VulkanRenderer::createTexture() {
    simpleDescriptorManager.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleDescriptorManager.bindDevice = mainDevice.logicalDevice;
    simpleDescriptorManager.bindQueue = mainDevice.graphicsQueue;
    simpleDescriptorManager.bindCommandPool =simpleCommandManager.graphicsCommandPool;
    simpleDescriptorManager.bindSwapChainExtent = &simpleSwapchain.swapChainExtent;
    //simpleDescriptorManager.createTexture("content/viking_room.png");
    simpleDescriptorManager.createTexture("content/veqhch1/veqhch1_4K_Albedo.jpg");
}
void VulkanRenderer::loadModel() {
    //simpleObjLoader.readFile("content/pig.obj");
    //simpleObjLoader.readFile("content/viking_room.obj");
    simpleObjLoader.readFile("content/veqhch1/veqhch1_LOD0.obj");
    //simpleQuad.init();
}



void VulkanRenderer::createVertexBuffer() {
    simpleVertexBuffer.bindDevice = mainDevice.logicalDevice;
    simpleVertexBuffer.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleVertexBuffer.bindQueue = mainDevice.graphicsQueue;
    simpleVertexBuffer.bindCommandPool = simpleCommandManager.graphicsCommandPool;
    /*
    simpleVertexBuffer.createVertexBufferWithStagingBuffer(sizeof(Vertex) * simpleObjLoader.vertices.size(),
        simpleObjLoader.vertices.data());
    simpleVertexBuffer.createIndexBuffer(sizeof(uint32_t)* simpleObjLoader.indices.size(),
        simpleObjLoader.indices.data());*/


    //render quad
    /*
   simpleVertexBuffer.createVertexBufferWithStagingBuffer(sizeof(Vertex) * simpleQuad.vertices.size(),
       simpleQuad.vertices.data());
   simpleVertexBuffer.createIndexBuffer(sizeof(uint32_t)* simpleQuad.indices.size(),
       simpleQuad.indices.data());*/

    //createVertexAndIndexBuffer(simpleVertexBuffer, simpleQuad);
    createVertexAndIndexBuffer(simpleVertexBuffer, simpleObjLoader);
}

void VulkanRenderer::createUniformBuffers() {
    simpleDescriptorManager.bindDevice = mainDevice.logicalDevice;
    simpleDescriptorManager.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleDescriptorManager.bindSwapChainExtent = &simpleSwapchain.swapChainExtent;
    simpleDescriptorManager.createUniformBuffers();
}

void VulkanRenderer::createDescriptorPool() {
    simpleDescriptorManager.bindDevice = mainDevice.logicalDevice;
    simpleDescriptorManager.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleDescriptorManager.bindSwapChainExtent = &simpleSwapchain.swapChainExtent;
    simpleDescriptorManager.createDescriptorPool();
}

void VulkanRenderer::createDescriptorSets() {
    simpleDescriptorManager.bindDevice = mainDevice.logicalDevice;
    simpleDescriptorManager.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleDescriptorManager.bindSwapChainExtent = &simpleSwapchain.swapChainExtent;
    simpleDescriptorManager.createDescriptorSets();
}


void VulkanRenderer::createCommandBuffers() {
    simpleCommandManager.bindLogicDevice = mainDevice.logicalDevice;
    simpleCommandManager.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleCommandManager.bindSurface = surfaceKhr;
    simpleCommandManager.bindSwapChainFramebuffers = &simpleFramebuffer.swapChainFramebuffers;
    simpleCommandManager.bindRenderPass = simplePass.pass;
    simpleCommandManager.bindSwapChainExtent = &simpleSwapchain.swapChainExtent;
    simpleCommandManager.bindPipeline = simplePipeline.graphicsPipeline;
    simpleCommandManager.bindPipeLineLayout = simplePipeline.pipelineLayout;
    simpleCommandManager.bindDescriptorSets = &simpleDescriptorManager.descriptorSets;
    simpleCommandManager.bindCurrentFrame = &currentFrame;
    simpleCommandManager.createCommandBuffers();

    /* DRAW TRANGLE
    std::vector          vertexBuffer = { simpleVertexBuffer.createdBuffers[0].buffer};
    std::vector<VkDeviceSize> offsets = {0};
    simpleCommandManager.bindVertexBuffers = {
        std::move(vertexBuffer),
        std::move(offsets),
        0,1, 3
    };
    simpleCommandManager.createCommandBuffers();*/


    /*
    std::vector          vertexBuffer = { simpleVertexBuffer.createdBuffers[0].buffer};
    std::vector<VkDeviceSize> offsets = {0};
    simpleCommandManager.bindVertexBuffers = {
        std::move(vertexBuffer),
        std::move(offsets),
        0,1, simpleObjLoader.vertices.size()
    };
    simpleCommandManager.bindIndexBuffer = {
        simpleVertexBuffer.createdIndexedBuffers[0].buffer,
        0,
        VK_INDEX_TYPE_UINT32,
        simpleObjLoader.indices.size()
    };*/

    /*
    std::vector          vertexBuffer = { simpleVertexBuffer.createdBuffers[0].buffer};
    std::vector<VkDeviceSize> offsets = {0};
    simpleCommandManager.bindVertexBuffers = {
        std::move(vertexBuffer),
        std::move(offsets),
        0,1, simpleQuad.vertices.size()
    };
    simpleCommandManager.bindIndexBuffer = {
        simpleVertexBuffer.createdIndexedBuffers[0].buffer,
        0,
        VK_INDEX_TYPE_UINT32,
        simpleQuad.indices.size()
    };*/
}




void VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_CIO{};
    semaphore_CIO.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_CIO{};
    fence_CIO.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_CIO.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for(int i=0;i<MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(mainDevice.logicalDevice, &semaphore_CIO, nullptr, &imageAvailableSemaphores[i]);
        vkCreateSemaphore(mainDevice.logicalDevice, &semaphore_CIO, nullptr, &renderFinishedSemaphores[i]);
        vkCreateFence(mainDevice.logicalDevice, &fence_CIO, nullptr, &inFlightFences[i]);
    }
}


void VulkanRenderer::draw() {
    //std::cout << "draw frame:" << currentFrame << std::endl;
    // GPU -> CPU, 等待上一帧是不是渲染完成
    vkWaitForFences(mainDevice.logicalDevice, 1, &inFlightFences[currentFrame],VK_TRUE,UINT64_MAX);

    //std::cout << "frame:" << currentFrame << std::endl;
    // GPU->GPU 获取可以绘制的交换链图片
    uint32_t imageIndex;
    auto result =vkAcquireNextImageKHR(mainDevice.logicalDevice, simpleSwapchain.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }else if(result !=VK_SUCCESS and result!=VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error{"failed to acquire swap chain image!"};
    }
    simpleDescriptorManager.simpleUniformBuffer.updateUniform(currentFrame);
    vkResetFences(mainDevice.logicalDevice, 1, &inFlightFences[currentFrame]);

    VkCommandBuffer commandBufferToSubmit = simpleCommandManager.commandBuffers[currentFrame];
    vkResetCommandBuffer(commandBufferToSubmit,/*VkCommandBufferResetFlagBits*/ 0);
    //simpleCommandManager.recordCommand(commandBufferToSubmit, imageIndex);
    //simpleCommandManager.recordCommandWithGeometry(simpleQuad, commandBufferToSubmit, imageIndex);
    simpleCommandManager.recordCommandWithGeometry(simpleObjLoader, commandBufferToSubmit, imageIndex);
    //std::cout << "record:" << currentFrame << std::endl;
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount  = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame]; // 如果交换链图片已经准备好信号
    constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBufferToSubmit;
    // 渲染完成信号，发射renderFinishedSemaphores
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];
    if (vkQueueSubmit(mainDevice.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    //std::cout << "submit:" << currentFrame << std::endl;
    // present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame]; // render完成
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &simpleSwapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    result = vkQueuePresentKHR(mainDevice.presentationQueue, &presentInfo);

    if(result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR or framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    }else if(result != VK_SUCCESS) {
        throw std::runtime_error{"failed to present swapchain image"};
    }

    // advanced
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

