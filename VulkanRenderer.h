
#ifndef CP_01_VULKANRENDERER_H
#define CP_01_VULKANRENDERER_H

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "RenderPass.h"
#include "Utils.h"
#include "VulkanValidation.h"
#include "Swapchain.h"
#include "Frambuffer.h"
#include "CommandManager.h"
#include "Device.h"
#include "BufferManager.h"
#include "DescriptorManager.h"
#include "GeoVertexDescriptions.h"
class VulkanRenderer {
public :
    VulkanRenderer();
    void initWindow();
    int initVulkan();
    void mainLoop();
    void cleanup();
    void run();
    void draw();
private:
    bool framebufferResized  = false;
    GLFWwindow  *window;
    VkInstance instance{};
    bool enableValidation{true};
    VkDebugReportCallbackEXT reportCallback;

    Device mainDevice;
    VkSurfaceKHR surfaceKhr;

    // Objects
    DebugV2::CustomDebug simpleDebug;
    Pipeline simplePipeline;
    RenderPass simplePass;
    Swapchain simpleSwapchain;
    Frambuffer simpleFramebuffer;
    CommandManager simpleCommandManager;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    int currentFrame{0};
    BufferManager simpleVertexBuffer;
    DescriptorManager simpleDescriptorManager;
    ImageAndMemory depthImageAndMemory;
    VkImageView depthImageView;
    ObjLoader simpleObjLoader{};
    Quad simpleQuad{};


    // create functions
    void createInstance();
    void createDebugCallback();
    void createPhyiscalAndLogicDevice();
    void createSurface();
    void createSwapChain();
    void createRenderpass();
    void createDescriptorSetLayout();
    void createPipeline();
    void createDepthResources();
    void createFramebuffers();
    void createCommandPool();
    void createTexture();
    void loadModel();
    void createVertexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    // swapchain recreate
    void cleanupSwapChain();
    void recreateSwapChain();
    static void checkInstanceExtensionSupport(const std::vector<const char *> &checkExtensions);



};
#endif //CP_01_VULKANRENDERER_H
