#include <iostream>
#include <GLFW/glfw3.h>
#include "VulkanRenderer.h"
#include "renderer/BasicRenderer.h"
int main() {
    BasicRenderer app;
    try {
        app.run();
    }
    catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
