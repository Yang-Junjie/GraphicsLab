cmake_minimum_required(VERSION 3.15)
project(CGLab)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin CACHE PATH "Output directory for binaries")

add_subdirectory(third_party/Flux)
add_subdirectory(third_party/simdjson)
add_subdirectory(third_party/fastgltf)

add_executable(CGLab
    src/App.cpp
    src/AppLayer.cpp
    src/JobSystem/JobSystem.cpp
    src/Backend/OpenGL/Buffer.cpp
    src/Backend/OpenGL/Framebuffer.cpp
    src/Backend/OpenGL/Shader.cpp
    src/Backend/OpenGL/VertexArray.cpp
    src/Renderer/Renderer2D.cpp
    src/Model/Material.cpp
    src/Model/Mesh.cpp
    src/Model/Model.cpp
    src/Scene/Scenes/TriangleScene.cpp
    src/Scene/Scenes/StressTestScene.cpp
    src/Scene/Scenes/TextureScene.cpp
    src/Scene/Scenes/CameraTestScene.cpp
    src/Scene/Scenes/BaseLighting.cpp
    src/Scene/Scenes/AdvanceLighting/AdvanceLighting.cpp
    src/Scene/Scenes/AdvanceLighting/AdvanceLightingState.cpp
    src/Scene/Scenes/PBR/PBR.cpp
    src/Scene/Scenes/PBR/PBRState.cpp
    src/Scene/Scenes/ModelScene.cpp
    src/Scene/Scenes/BezierCurveScene.cpp
    src/Scene/Scenes/BezierSurfaceScene.cpp
    third_party/stb/stb_image.cpp
)

set_target_properties(CGLab PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
)

target_include_directories(CGLab PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/stb
)
target_link_libraries(CGLab PRIVATE
    FluxCore
    fastgltf::fastgltf
)

if(MSVC)
    target_compile_options(CGLab PRIVATE /utf-8)
endif()

# Copy shaders to output directory
add_custom_command(TARGET CGLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:CGLab>/shaders
)

# Copy resources to output directory
add_custom_command(TARGET CGLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res
        $<TARGET_FILE_DIR:CGLab>/res
)
