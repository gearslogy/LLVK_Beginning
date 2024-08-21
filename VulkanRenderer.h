
#ifndef CP_01_VULKANRENDERER_H
#define CP_01_VULKANRENDERER_H

#include <vma/vk_mem_alloc.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "RenderPass.h"
#include "VulkanValidation.h"
#include "Swapchain.h"
#include "Frambuffer.h"
#include "Device.h"
#include "PipelineCache.h"
#include "LLVK_Image.h"
#include "LLVK_Camera.h"

LLVK_NAMESPACE_BEGIN


struct VulkanRendererWindowEvent {
    static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn );// Callback function for mouse movement
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void process_input(GLFWwindow* window);
};



class VulkanRenderer {
    friend class VulkanRendererWindowEvent;
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

    // When rendering, you can only pick one from these asynchronous resources
    VkFramebuffer activatedSwapChainFramebuffer{};          // swapchain frame buffer. we have three images in our swapchain
    VkCommandBuffer activatedFrameCommandBufferToSubmit{};  // [two command buffer,MAX_FLIGHT=2]  only one frame activated
    VkSemaphore activatedImageAvailableSemaphore{};         // [two semaphores,  MAX_FLIGHT=2]    only one frame activated
    VkSemaphore activatedRenderFinishedSemaphore{};         // [two semaphores,  MAX_FLIGHT=2]    only one frame activated
    VmaAllocator vmaAllocator{};
    Camera mainCamera{};
    float dt = 0.0f;
    float lastFrameTime = 0.0f;




    VkSubmitInfo submitInfo{};
    // create functions
    void createInstance();
    void createDebugCallback();
    void createPhyiscalAndLogicDevice();
    void createVmaAllocator();
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
    virtual void prepare() {}
    virtual void render() = 0;
public:
    struct {
        bool isPressingRightMouseButton{false};
        bool isPressingLeftMouseButton{false};
        bool firstMouse{true};
        float lastX = 400, lastY = 300; // prevframe mouse position
    }eventData;

};
LLVK_NAMESPACE_END
#endif //CP_01_VULKANRENDERER_H
