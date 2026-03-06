cmake_minimum_required(VERSION 3.15)
project(FluidSimulation)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin CACHE PATH "Output directory for binaries")

add_subdirectory(thrid_party/Flux)

find_package(Threads REQUIRED)

add_executable(FluidSimulation
    src/FluidSimApp.cpp
    src/Renderer/Renderer2D.cpp
    src/Simulation/ParticleSystem.cpp
)

set_target_properties(FluidSimulation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
)

target_include_directories(FluidSimulation PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(FluidSimulation PRIVATE FluxCore Threads::Threads)

if(MSVC)
    target_compile_options(FluidSimulation PRIVATE /utf-8)
endif()
