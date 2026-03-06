cmake_minimum_required(VERSION 3.15)
project(FluidSimulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin CACHE PATH "Output directory for binaries")

add_subdirectory(thrid_party/Flux)

add_executable(FluidSimulation src/FluidSimApp.cpp)

set_target_properties(FluidSimulation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
)

target_link_libraries(FluidSimulation PRIVATE FluxCore)

if(MSVC)
    target_compile_options(FluidSimulation PRIVATE /utf-8)
endif()
