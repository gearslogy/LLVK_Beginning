
//
// Created by liuyangping on 2024/3/27.
//
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include "VulkanRenderer.h"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <cassert>
#include <set>
#include "VulkanValidation.h"
#include "libs/magic_enum.hpp"
#include <format>
#include <filesystem>
#include "CommandManager.h"



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



LLVK_NAMESPACE_BEGIN
// control camera rotation
void VulkanRendererWindowEvent::mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
    /*
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS)
        return;*/

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    auto vr = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    if (not vr->eventData.isPressingRightMouseButton) return;

    /*
    if (vr->eventData.firstMouse) {
        std::cout << "first pos in\n";
        vr->eventData.lastX = xpos;
        vr->eventData.lastY = ypos;
        vr->eventData.firstMouse = false;
    }*/
    float xoffset = xpos - vr->eventData.lastX;
    float yoffset = vr->eventData.lastY - ypos;
    //xoffset = std::clamp(xoffset, -1.0f , 1.0f);
    //yoffset = std::clamp(yoffset, -1.0f , 1.0f);
    vr->eventData.lastX = xpos;
    vr->eventData.lastY = ypos;
    vr->mainCamera.processMouseMovement(xoffset, yoffset); // camera yaw and pitch

}
// if glfw is pressing mouse right button
void VulkanRendererWindowEvent::mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            auto vr = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
            vr->eventData.isPressingRightMouseButton = true;
            // ---key !
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            vr->eventData.lastX = static_cast<float>(xpos);
            vr->eventData.lastY = static_cast<float>(ypos);
            // ---or cause camera jitter when mouse clicked
            //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        } else if (action == GLFW_RELEASE) {
            //std::cout << "released\n";;
            auto vr = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
            vr->eventData.isPressingRightMouseButton = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void VulkanRendererWindowEvent::scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    auto vr = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    vr->mainCamera.processMouseScroll(static_cast<float>(yoffset));
}

void VulkanRendererWindowEvent::process_input(GLFWwindow *window) {
    auto vr = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    auto &cam = vr->mainCamera;
    const auto &dt = vr->dt;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam.processKeyboard(Camera::Camera_Movement::FORWARD, dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam.processKeyboard(Camera::Camera_Movement::BACKWARD, dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam.processKeyboard(Camera::Camera_Movement::LEFT, dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam.processKeyboard(Camera::Camera_Movement::RIGHT, dt);
}

void VulkanRendererWindowEvent::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto vr = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    vr->keyPressEvent(key,scancode, action, mods);
}



VulkanRenderer::VulkanRenderer()=default;
VulkanRenderer::~VulkanRenderer(){};



void VulkanRenderer::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(1920, 1080, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height ) {
        auto app = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
        std::cout << "glfw windows size changed:" << width << " " << height << std::endl;
        app->framebufferResized = true;
    });
    glfwSetCursorPosCallback(window,VulkanRendererWindowEvent::mouse_callback);
    glfwSetMouseButtonCallback(window, VulkanRendererWindowEvent::mouse_button_callback);
    glfwSetScrollCallback(window, VulkanRendererWindowEvent::scroll_callback);
    glfwSetKeyCallback(window, VulkanRendererWindowEvent::key_callback);

}
void VulkanRenderer::mainLoop() {
    while (!glfwWindowShouldClose(window)){
        float currentFrame = static_cast<float>(glfwGetTime());
        dt = currentFrame - lastFrameTime;
        lastFrameTime = currentFrame;
        VulkanRendererWindowEvent::process_input(window);
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
        createVmaAllocator();
        createSwapChain();
        createRenderpass();
        createPipelineCache();
        createDepthResources();
        createFramebuffers();
        createCommandPool();
        prepare();
        simplePipelineCache.writeCache();
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
    cleanupObjects();
    cleanupSwapChain();
    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        vkDestroySemaphore(mainDevice.logicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(mainDevice.logicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(mainDevice.logicalDevice,inFlightFences[i], nullptr);
    }
    simplePipelineCache.cleanup();
    vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
    vmaDestroyAllocator(vmaAllocator);
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
    swapChainResize();
}




void VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "TestEngineApp",
            .applicationVersion = VK_VERSION_1_3,
            .pEngineName = "TestEngine",
            .engineVersion = VK_API_VERSION_1_3,
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
    DebugV2::CommandLabel::init(instance);
}

void VulkanRenderer::createPhyiscalAndLogicDevice() {
    mainDevice.bindSurface = surfaceKhr;
    mainDevice.bindInstance = instance;
    mainDevice.init();
}
void VulkanRenderer::createVmaAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorInfo.physicalDevice = mainDevice.physicalDevice;
    allocatorInfo.device = mainDevice.logicalDevice;
    allocatorInfo.instance = instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    vmaCreateAllocator(&allocatorInfo, &vmaAllocator);
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

void VulkanRenderer::createPipelineCache() {
    simplePipelineCache.bindDevice = mainDevice.logicalDevice;
    simplePipelineCache.init();
    simplePipelineCache.loadCache();// actully it's trying load cache
}

void VulkanRenderer::createDepthResources() {
    auto depthFormat = FnImage::findDepthFormat(mainDevice.physicalDevice);
    FnImage::createImageAndMemory(mainDevice.physicalDevice,mainDevice.logicalDevice,
        simpleSwapchain.swapChainExtent.width,
        simpleSwapchain.swapChainExtent.height,
        1,1,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImageAndMemory.image, depthImageAndMemory.memory
    );
    FnImage::createImageView(mainDevice.logicalDevice,
        depthImageAndMemory.image,
        depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,1,1, depthImageView);
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
    QueueFamilyIndices queueFamilyIndices = getQueueFamilies(surfaceKhr, mainDevice.physicalDevice);
    graphicsCommandPool = FnCommand::createCommandPool(mainDevice.logicalDevice, queueFamilyIndices.graphicsFamily);
}

void VulkanRenderer::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    std::cout << __FUNCTION__ << " create swapChainCommandBuffers MAX_FRAMES_IN_FLIGHT size:" << commandBuffers.size() << std::endl;
    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = graphicsCommandPool ;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());
    if (result != VK_SUCCESS){
        throw std::runtime_error("Failed to allocate Command Buffers!");
    }
}


void VulkanRenderer::createSyncObjects() {
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = waitStages;

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
    vkWaitForFences(mainDevice.logicalDevice, 1, &inFlightFences[currentFlightFrame],VK_TRUE,UINT64_MAX);

    //std::cout << "frame:" << currentFrame << std::endl;
    // GPU->GPU 获取可以绘制的交换链图片
    auto result =vkAcquireNextImageKHR(mainDevice.logicalDevice, simpleSwapchain.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFlightFrame], VK_NULL_HANDLE, &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }else if(result !=VK_SUCCESS and result!=VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error{"failed to acquire swap chain image!"};
    }
    // here can push constant and update uniform
    vkResetFences(mainDevice.logicalDevice, 1, &inFlightFences[currentFlightFrame]);
    vkResetCommandBuffer(commandBuffers[currentFlightFrame], 0); //0: main command buffer reset
    render(); // call the pure virtual function(record command buffer)                         //1: command buffer new content
    // advanced
    currentFlightFrame = (currentFlightFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkPipelineCache VulkanRenderer::getPipelineCache() const {
    return simplePipelineCache.pipelineCache;
}



LLVK_NAMESPACE_END
