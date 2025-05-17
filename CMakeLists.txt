cmake_minimum_required(VERSION 3.24)
project(CP_02)

set(CMAKE_CXX_STANDARD 26)
set(GLFW_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64/)
set(GLFW_PATH_INCLUDE ${GLFW_PATH}/include)
set(GLFW_PATH_LIB ${GLFW_PATH}/lib-vc2022)


find_package(Vulkan)
file(GLOB SOURCES *.h *.cpp
        renderer/*
        renderer/*/*
        renderer/*/*/*
        ${CMAKE_SOURCE_DIR}/libs/tinyexr/deps/miniz/miniz.c
)

add_executable(${PROJECT_NAME} main.cpp ${SOURCES}
        renderer/epic/EpicRenderer.cpp
        renderer/epic/EpicRenderer.h
        renderer/epic/EpicRendererDefines.hpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL)
# GLM_ENABLE_EXPERIMENTAL for hash: #include <glm/gtx/hash.hpp>
MESSAGE("VK_SDK_PATH Path----> $ENV{VK_SDK_PATH}" )
MESSAGE("VULKAN_SDK  Path----> $ENV{VULKAN_SDK}" )

set(glsl_compiler_exec_path $ENV{VK_SDK_PATH}/bin/glslc.exe)
set(shader_include_folder ${CMAKE_SOURCE_DIR}/shaders/include)
file(GLOB shader_include_files ${shader_include_folder}/*)
set(compile_shader_folder ${CMAKE_SOURCE_DIR}/build/bin/shaders)
file(MAKE_DIRECTORY ${compile_shader_folder}) # 确保输出目录存在
file(GLOB_RECURSE shader_sources
        shaders/*.frag
        shaders/*.geom
        shaders/*.vert
        shaders/dynamicsUBO/*.frag
        shaders/dynamicsUBO/*.vert
        shaders/quad/*.frag
        shaders/quad/*.vert
)

foreach(shader_source IN LISTS shader_sources)
    get_filename_component(shader_name "${shader_source}" NAME_WE)
    get_filename_component(shader_ext "${shader_source}" EXT)
    set(output_path "${compile_shader_folder}/${shader_name}.spv")
    set(CMD ${glsl_compiler_exec_path} -I ${shader_include_folder} ${shader_source} -o ${output_path})
    MESSAGE("[[AOT SHADER]]: ${output_path}")
    MESSAGE("[[AOT COMMAND:]] ${CMD}")
    #我们需要使用 add_custom_command 配合 OUTPUT 来指定一个输出文件，
    #这样 CMake 才能正确地跟踪文件的依赖关系，并在源文件发生变化时重新生成输出文件。
    add_custom_command(
            OUTPUT ${output_path}
            DEPENDS ${shader_source} ${shader_include_files}
            COMMAND ${CMD}
            COMMENT "Compiling shader >> ${shader_name}${shader_ext}"
    )
    # 为了确保自定义命令在构建目标之前执行，我们需要创建一个自定义目标
    # 并将其作为项目的依赖项
    add_custom_target(${shader_name}_spv DEPENDS ${output_path})
    add_dependencies(${PROJECT_NAME} ${shader_name}_spv)
endforeach()
MESSAGE("-------->" ${Vulkan_LIBRARIES}) # vulkan-1.lib

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}
        ${GLFW_PATH_INCLUDE}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/libs/KTX-Software/include) # ktx include
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/libs/tinyexr/deps/miniz) # miniz for tinyexr
target_link_directories(${PROJECT_NAME} PUBLIC
        ${GLFW_PATH_LIB}
        ${CMAKE_CURRENT_LIST_DIR}/libs/KTX-Software/lib) # ktx lib dir
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw3 ktx)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
target_compile_definitions(${PROJECT_NAME} PRIVATE MAGIC_ENUM_RANGE_MIN=-200 MAGIC_ENUM_RANGE_MAX=500 )

add_subdirectory(tools)