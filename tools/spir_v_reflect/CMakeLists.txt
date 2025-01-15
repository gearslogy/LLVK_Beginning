


message("spriv_v_reflect source dir:" ${CMAKE_SOURCE_DIR})

file(TO_CMAKE_PATH "$ENV{VK_SDK_PATH}" VULKAN_SDK)

set(SPIRV_REFLECT_PATH "${VULKAN_SDK}/Source/SPIRV-Reflect")
if(NOT EXISTS "${SPIRV_REFLECT_PATH}")
    message(FATAL_ERROR "Could not find SPIRV-Reflect at: ${SPIRV_REFLECT_PATH}")
endif()

set(SPIRV_REFLECT_SOURCE "${SPIRV_REFLECT_PATH}/spirv_reflect.c")
if(NOT EXISTS "${SPIRV_REFLECT_SOURCE}")
    message(FATAL_ERROR "Could not find spirv_reflect.c at: ${SPIRV_REFLECT_SOURCE}")
endif()


add_executable(spirv_reflection spirv_reflection.cpp "${SPIRV_REFLECT_SOURCE}")
set_target_properties(spirv_reflection PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(spirv_reflection PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(spirv_reflection PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
target_include_directories(spirv_reflection PRIVATE ${CMAKE_SOURCE_DIR} ${SPIRV_REFLECT_PATH})
target_include_directories(spirv_reflection PRIVATE ${Vulkan_INCLUDE_DIRS}) # ktx include
target_compile_definitions(spirv_reflection PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL)




set(compile_shader_folder ${CMAKE_SOURCE_DIR}/build/bin/shaders)
set(shader_compile_path $ENV{VK_SDK_PATH}/bin/glslc.exe)
set(shader_source ${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect_vert.vert)
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
add_dependencies(spirv_reflection ${shader_name}_spv)
