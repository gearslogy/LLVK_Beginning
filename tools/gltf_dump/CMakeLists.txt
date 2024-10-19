
add_executable(gltf_dump gltf_dump.cpp)

set_target_properties(gltf_dump PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(gltf_dump PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(gltf_dump PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)

message("gltf_dump source dir:" ${CMAKE_SOURCE_DIR})
target_include_directories(gltf_dump PRIVATE ${CMAKE_SOURCE_DIR})

target_include_directories(gltf_dump PRIVATE ${Vulkan_INCLUDE_DIRS}) # ktx include
target_compile_definitions(gltf_dump PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL)