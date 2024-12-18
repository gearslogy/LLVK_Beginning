
set_source_files_properties(${CMAKE_SOURCE_DIR}/libs/tinyexr/deps/miniz/miniz.c PROPERTIES LANGUAGE C)
add_executable(exr_dump exr_dump.cpp ${CMAKE_SOURCE_DIR}/libs/tinyexr/deps/miniz/miniz.c )

set_target_properties(exr_dump PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)
set_target_properties(exr_dump PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set_target_properties(exr_dump PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)

message("exr_dump source dir:" ${CMAKE_SOURCE_DIR})
target_include_directories(exr_dump PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(exr_dump PRIVATE ${CMAKE_SOURCE_DIR}/libs/tinyexr/deps/miniz) # ktx include
target_include_directories(exr_dump PRIVATE ${Vulkan_INCLUDE_DIRS}) # ktx include
target_compile_definitions(exr_dump PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL)