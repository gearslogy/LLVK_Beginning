
#ifndef CP_01_VULKANRENDERER_H
#define CP_01_VULKANRENDERER_H

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "RenderPass.h"
#include "Utils.h"
#include "VulkanValidation.h"
#include "Swapchain.h"
#include "Frambuffer.h"
#include "CommandManager.h"
#include "Device.h"
#include "BufferManager.h"
#include "DescriptorManager.h"
#include "PipelineCache.h"

class VulkanRenderer {
public :
    VulkanRenderer();
    virtual ~VulkanRenderer() ;
    void initWindow();
    int initVulkan();
    void mainLoop();
    void cleanup();
    void run();
    void draw();
protected:
    bool framebufferResized  = false;
    GLFWwindow  *window;
    VkInstance instance{};
    bool enableValidation{true};
    VkDebugReportCallbackEXT reportCallback;

    Device mainDevice;
    VkSurfaceKHR surfaceKhr;

    // Objects
    DebugV2::CustomDebug simpleDebug;
    //Pipeline simplePipeline;
    RenderPass simplePass;
    Swapchain simpleSwapchain;
    Frambuffer simpleFramebuffer;
    VkCommandPool graphicsCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;// Resize command buffer count to have one for each framebuffer
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    int currentFrame{0};
    ImageAndMemory depthImageAndMemory;
    VkImageView depthImageView;
    PipelineCache simplePipelineCache{};
    VkFramebuffer activeSwapChainFramebuffer{}; // swapchain frame buffer. we have three images in our swapchain
    VkCommandBuffer activedFrameCommandBuferToSubmit{}; // we have two command buffer.active for the render rerord-command
    // create functions
    void createInstance();
    void createDebugCallback();
    void createPhyiscalAndLogicDevice();
    void createSurface();
    void createSwapChain();
    void createRenderpass();
    //void createDescriptorSetLayout(); //user
    void createPipelineCache();
    //void createPipeline();            // user
    void createDepthResources();
    void createFramebuffers(); // call at : init() recreateSwapChain()
    void createCommandPool();

    void createCommandBuffers();
    void createSyncObjects();

    // swapchain recreate
    void cleanupSwapChain();
    void recreateSwapChain();
    static void checkInstanceExtensionSupport(const std::vector<const char *> &checkExtensions);

    // interface
    virtual void cleanupObjects(){};
    virtual void recordCommandBuffer(){};
    virtual void prepare() {}
    virtual void render() = 0;


};
#endif //CP_01_VULKANRENDERER_H
