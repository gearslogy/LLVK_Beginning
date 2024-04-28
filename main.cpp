#include <iostream>
#include <GLFW/glfw3.h>
#include "VulkanRenderer.h"

inline auto* initWindow(){
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    return glfwCreateWindow(800,600, "Vulkan", nullptr, nullptr);
}

int main() {
    GLFWwindow *window = initWindow();
    VulkanRenderer renderer;
    if(renderer.init(window) == EXIT_FAILURE){
        return EXIT_FAILURE;
    }
    while (!glfwWindowShouldClose(window)){
        glfwPollEvents();
    }
    renderer.cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
