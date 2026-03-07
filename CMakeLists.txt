cmake_minimum_required(VERSION 3.15)
project(FluidSimulation)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin CACHE PATH "Output directory for binaries")

add_subdirectory(thrid_party/Flux)

add_executable(FluidSimulation
    src/FluidSimApp.cpp
    src/Renderer/Renderer2D.cpp
)

set_target_properties(FluidSimulation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
)

target_include_directories(FluidSimulation PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(FluidSimulation PRIVATE FluxCore)

if(MSVC)
    target_compile_options(FluidSimulation PRIVATE /utf-8)
endif()

# Copy shaders to output directory
add_custom_command(TARGET FluidSimulation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:FluidSimulation>/shaders
)
