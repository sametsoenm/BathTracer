cmake_minimum_required(VERSION 3.24)
project(BathTracerTwo LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

find_package(CUDAToolkit REQUIRED)

include(FetchContent)

add_subdirectory(external/glm)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

add_library(glad STATIC
    external/glad/src/glad.c
 "cuda/device_types.h" "cuda/util/vec_math.h" "cuda/util/random.h" "cuda/util/onb.h" "cuda/util/sampling.h" "cuda/util/color.h"  "cuda/materials/lambert_diffuse.h" "cuda/materials/material.h" "cuda/materials/mirror.h" "cuda/util/util.h" "cuda/materials/dielectric.h" "cuda/materials/material_funcs.h" "cuda/materials/specular_microfacet.h" "cuda/volume.h" "cuda/shading.h" "cuda/light.h")

target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/external/glad/include
)

add_subdirectory(src)

