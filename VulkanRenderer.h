
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
#include "MainFramebuffer.h"
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
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};



class VulkanRenderer {
    friend struct VulkanRendererWindowEvent;
public :
    VulkanRenderer();
    virtual ~VulkanRenderer() ;
    void initWindow();
    int initVulkan();
    void mainLoop();
    void cleanup();
    void run();
    void draw();

    template<class Self>
    auto&& getMainDevice(this Self& self) { return std::forward<Self>(self).mainDevice;}
    template<class Self>
    auto&& getGraphicsCommandPool(this Self&self) {return std::forward<Self>(self).graphicsCommandPool;}
    template<class Self>
    auto&& getGraphicsQueue(this Self&self) {return std::forward<Self>(self).getMainDevice().graphicsQueue;}
    template<class Self>
    auto&& getVmaAllocator(this Self&self) {return std::forward<Self>(self).vmaAllocator;}

    template<class Self> auto&& getMainCamera(this Self&self)  {return std::forward<Self>(self).mainCamera;}

    [[nodiscard]] auto getSwapChainExtent() const { return simpleSwapchain.swapChainExtent;}
    [[nodiscard]] VkPipelineCache getPipelineCache() const ;
    [[nodiscard]] VkRenderPass getMainRenderPass() const { return simplePass.pass;}

    // Get FrameBuffer
    template <typename Self>
    [[nodiscard]] auto&& getMainFramebuffer(this Self&& self)  {
        return std::forward<Self>(self).simpleFramebuffer.swapChainFramebuffers[self.imageIndex];
    }

    template <typename Self>
    [[nodiscard]] auto&& getMainCommandBuffer(this Self&& self)  {
        return std::forward<Self>(self).commandBuffers[self.currentFlightFrame];
    }

    // Current Flight Frame
    [[nodiscard]] auto getCurrentFlightFrame() const{ return currentFlightFrame;}
protected:
    bool framebufferResized  = false;
    GLFWwindow  *window;
    VkInstance instance{};
#ifdef _DEBUG
    bool enableValidation{true};
#else
    bool enableValidation{true};
#endif
    VkDebugReportCallbackEXT reportCallback;

    Device mainDevice;
    VkSurfaceKHR surfaceKhr;

    // Objects
    DebugV2::CustomDebug simpleDebug;
    //Pipeline simplePipeline;
    RenderPass simplePass;
    Swapchain simpleSwapchain;
    MainFramebuffer simpleFramebuffer;
    VkCommandPool graphicsCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;// Resize command buffer count to have one for each framebuffer
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    int currentFlightFrame{0};
    ImageAndMemory depthImageAndMemory;
    VkImageView depthImageView;
    PipelineCache simplePipelineCache{};

    // When rendering, you can only pick one from these asynchronous resources
    //VkFramebuffer activatedSwapChainFramebuffer{};          // swapchain frame buffer. we have three images in our swapchain
    //VkCommandBuffer activatedFrameCommandBufferToSubmit{};  // [two command buffer,MAX_FLIGHT=2]  only one frame activated
    //VkSemaphore activatedImageAvailableSemaphore{};         // [two semaphores,  MAX_FLIGHT=2]    only one frame activated
    //VkSemaphore activatedRenderFinishedSemaphore{};         // [two semaphores,  MAX_FLIGHT=2]    only one frame activated
    uint32_t imageIndex{};                                  // may be 0 1 2 based on swapchain images
    VmaAllocator vmaAllocator{};
    Camera mainCamera{};
    float dt = 0.0f;
    float lastFrameTime = 0.0f;





    // create functions
    void createInstance();
    void createDebugCallback();
    void createPhyiscalAndLogicDevice();
    void createVmaAllocator();
    void createSurface();
    void createSwapChain();

    //void createDescriptorSetLayout(); //user
    void createPipelineCache();
    //void createPipeline();            // user
    void createDepthResources();

    void createCommandPool();

    void createCommandBuffers();
    void createSyncObjects();

    // swapchain recreate
    void cleanupSwapChain();
    void recreateSwapChain();
    static void checkInstanceExtensionSupport(const std::vector<const char *> &checkExtensions);


    inline static constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};


    inline void submitTask(const VkCommandBuffer &cmdBuffer,
        const VkSemaphore &waitSemaphore,
        const VkSemaphore &signalSemaphore,
        const VkFence &fence = VK_NULL_HANDLE) {
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;
        // wait -> signal
        submitInfo.waitSemaphoreCount  = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore; // if this semaphore was signaled
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;
        if (vkQueueSubmit(mainDevice.graphicsQueue, 1, &submitInfo,fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit command buffer!");
        }
    }
    inline void presentFrame(const VkSemaphore &waitSemaphore) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore; // render完成
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &simpleSwapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        auto result = vkQueuePresentKHR(mainDevice.presentationQueue, &presentInfo);
        if(result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR or framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }else if(result != VK_SUCCESS) {
            throw std::runtime_error{"failed to present swapchain image"};
        }
    }

    // this can direct present
    void submitMainCommandBuffer() {
        submitTask(commandBuffers[currentFlightFrame],
             imageAvailableSemaphores[currentFlightFrame],
            renderFinishedSemaphores[currentFlightFrame],
            inFlightFences[currentFlightFrame]);
    }
    void presentMainCommandBufferFrame() {
        presentFrame(renderFinishedSemaphores[currentFlightFrame]);
    }



    // interface
    virtual void createRenderpass();
    virtual void createFramebuffers(); // call at : init() recreateSwapChain()
    virtual void cleanupObjects(){};
    virtual void prepare() {}
    virtual void render() = 0;
    virtual void swapChainResize() {}
    virtual void keyPressEvent(int key, int scanCode, int action, int mods) {};
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
