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
        Device*
        BufferManager*
        GeoVertexDescriptions*
        DescriptorManager*
)

add_executable(${PROJECT_NAME} main.cpp ${SOURCES})


MESSAGE("VK_SDK_PATH Path----> $ENV{VK_SDK_PATH}" )
MESSAGE("VULKAN_SDK  Path----> $ENV{VULKAN_SDK}" )

set(glsl_compiler_exec_path $ENV{VK_SDK_PATH}/bin/glslc.exe)

set(compile_shader_folder ${CMAKE_SOURCE_DIR}/build/bin/shaders)
file(MAKE_DIRECTORY ${compile_shader_folder}) # 确保输出目录存在
file(GLOB_RECURSE shader_sources shaders/*.frag shaders/*.vert)

foreach(shader_source IN LISTS shader_sources)
    get_filename_component(shader_name "${shader_source}" NAME_WE)
    get_filename_component(shader_ext "${shader_source}" EXT)
    set(output_path "${compile_shader_folder}/${shader_name}.spv")
    set(CMD ${glsl_compiler_exec_path}  ${shader_source} -o ${output_path})
    MESSAGE("[[AOT SHADER]]: ${output_path}")
    MESSAGE("[[AOT COMMAND:]] ${CMD}")
    #我们需要使用 add_custom_command 配合 OUTPUT 来指定一个输出文件，
    #这样 CMake 才能正确地跟踪文件的依赖关系，并在源文件发生变化时重新生成输出文件。
    add_custom_command(
            OUTPUT ${output_path}
            DEPENDS ${shader_source}
            COMMAND ${CMD}
            COMMENT "Compiling shader >> ${shader_name}${shader_ext}"
    )
    # 为了确保自定义命令在构建目标之前执行，我们需要创建一个自定义目标
    # 并将其作为项目的依赖项
    add_custom_target(${shader_name}_spv DEPENDS ${output_path})
    add_dependencies(${PROJECT_NAME} ${shader_name}_spv)
endforeach()


target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} ${GLFW_PATH_INCLUDE})
target_link_directories(${PROJECT_NAME} PUBLIC ${GLFW_PATH_LIB})
target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw3)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)


