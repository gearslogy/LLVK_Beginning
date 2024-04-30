cmake_minimum_required(VERSION 3.24)
project(CP_02)

set(CMAKE_CXX_STANDARD 23)
set(GLFW_PATH D:/plugin_dev/libs/glfw-3.4.bin.WIN64)
set(GLFW_PATH_INCLUDE ${GLFW_PATH}/include)
set(GLFW_PATH_LIB ${GLFW_PATH}/lib-vc2019)



FIND_PACKAGE(Vulkan)

file(GLOB SOURCES VulkanRenderer*
        RenderPass*
        Pipeline*
        Command*
        Swapchain*
        Image*
        Frambuffer*
)

add_executable(${PROJECT_NAME} main.cpp ${SOURCES})


MESSAGE("VK_SDK_PATH Path----> $ENV{VK_SDK_PATH}" )
MESSAGE("VULKAN_SDK  Path----> $ENV{VULKAN_SDK}" )

set(glsl_compiler_exec_path $ENV{VK_SDK_PATH}/bin/glslc.exe)

set(shader_folder ${CMAKE_SOURCE_DIR}/build/bin/shaders)
file(GLOB_RECURSE shader_sources
        ${shader_folder}/*.frag
        ${shader_folder}/*.vert)

foreach(shader_source IN LISTS shader_sources)
    get_filename_component(shader_name "${shader_source}" NAME_WE)
    get_filename_component(shader_ext "${shader_source}" EXT)
    set(output_path "${shader_folder}/${shader_name}.spv")
    MESSAGE("[[AOT SHADER]]: ${output_path}")
    MESSAGE("[[AOT COMMAND:]] ${glsl_compiler_exec_path}  ${shader_source} -o ${output_path}")

    add_custom_command( TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND "${glsl_compiler_exec_path}" ${shader_source} -o ${output_path}
            COMMENT "Compiling----------------> ${shader_name} ${shader_ext} with glslangValidator"
    )
endforeach()


target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} ${GLFW_PATH_INCLUDE})
target_link_directories(${PROJECT_NAME} PUBLIC ${GLFW_PATH_LIB})
target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw3)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)


