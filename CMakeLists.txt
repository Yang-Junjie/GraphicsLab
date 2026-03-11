cmake_minimum_required(VERSION 3.15)
project(CGDemo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin CACHE PATH "Output directory for binaries")

add_subdirectory(thrid_party/Flux)

add_executable(CGDemo
    src/App.cpp
    src/DemoLayer.cpp
    src/JobSystem/JobSystem.cpp
    src/Backend/OpenGL/Buffer.cpp
    src/Backend/OpenGL/Framebuffer.cpp
    src/Backend/OpenGL/Shader.cpp
    src/Backend/OpenGL/VertexArray.cpp
    src/Renderer/Renderer2D.cpp
    src/Scene/TriangleScene.cpp
    src/Scene/StressTestScene.cpp
    src/Scene/TextureScene.cpp
    thrid_party/stb/stb_image.cpp
)

set_target_properties(CGDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
)

target_include_directories(CGDemo PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/thrid_party/stb
)
target_link_libraries(CGDemo PRIVATE FluxCore)

if(MSVC)
    target_compile_options(CGDemo PRIVATE /utf-8)
endif()

# Copy shaders to output directory
add_custom_command(TARGET CGDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:CGDemo>/shaders
)

# Copy resources to output directory
add_custom_command(TARGET CGDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res
        $<TARGET_FILE_DIR:CGDemo>/res
)


