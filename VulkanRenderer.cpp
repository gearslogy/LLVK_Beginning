
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
        createPipeline();
        createFramebuffers();
        createCommandPoolAndBuffers();
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
    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        vkDestroySemaphore(mainDevice.logicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(mainDevice.logicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(mainDevice.logicalDevice,inFlightFences[i], nullptr);
    }

    simpleCommandManager.cleanup();
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

void VulkanRenderer::createPipeline(){
    simplePipeline.bindDevice = mainDevice.logicalDevice;
    simplePipeline.bindExtent = simpleSwapchain.swapChainExtent;
    simplePipeline.bindRenderPass = simplePass.pass;
    simplePipeline.init();
}
void VulkanRenderer::createRenderpass(){
    simplePass.bindDevice = mainDevice.logicalDevice;
    simplePass.init();
}
void VulkanRenderer::createFramebuffers() {
    simpleFramebuffer.bindDevice = mainDevice.logicalDevice;
    simpleFramebuffer.bindRenderPass = simplePass.pass;
    simpleFramebuffer.bindSwapChainExtent = simpleSwapchain.swapChainExtent;
    simpleFramebuffer.bindSwapChainImages = simpleSwapchain.swapChainImages;
    simpleFramebuffer.init();
}
void VulkanRenderer::createCommandPoolAndBuffers() {
    simpleCommandManager.bindLogicDevice = mainDevice.logicalDevice;
    simpleCommandManager.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleCommandManager.bindSurface = surfaceKhr;
    simpleCommandManager.bindSwapChainFramebuffers = &simpleFramebuffer.swapChainFramebuffers;
    simpleCommandManager.bindRenderPass = simplePass.pass;
    simpleCommandManager.bindSwapChainExtent = &simpleSwapchain.swapChainExtent;
    simpleCommandManager.bindPipeline = simplePipeline.graphicsPipeline;
    simpleCommandManager.init();
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
    vkResetFences(mainDevice.logicalDevice, 1, &inFlightFences[currentFrame]);

    VkCommandBuffer commandBufferToSubmit = simpleCommandManager.commandBuffers[currentFrame];
    vkResetCommandBuffer(commandBufferToSubmit,/*VkCommandBufferResetFlagBits*/ 0);
    simpleCommandManager.recordCommand(commandBufferToSubmit, imageIndex);
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
        std::cout << magic_enum::enum_name(result) <<"222\n";
    }else if(result != VK_SUCCESS) {
        throw std::runtime_error{"failed to present swapchain image"};
    }

    // advanced
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

