
# path to optix SDK
set(OPTIX_ROOT "C:/ProgramData/NVIDIA Corporation/OptiX SDK 9.1.0")
set(OPTIX_INCLUDE_DIR "${OPTIX_ROOT}/include")

if(NOT EXISTS "${OPTIX_INCLUDE_DIR}/optix.h")
    message(FATAL_ERROR "optix.h not found under: ${OPTIX_INCLUDE_DIR}")
endif()

add_executable(BathTracerTwo
    "main.cpp"
    "util/io.cpp"
 "optix/optix_stubs.cpp" 
 "app/path_tracer_app.cpp" "app/path_tracer_app.h" 
 "rendersettings.h" 
 "platform/display.cpp" "platform/display.h" 
 "platform/screen_quad.cpp" "platform/screen_quad.h" 
 "platform/shader_program.cpp" "platform/shader_program.h" 
 "platform/texture2d.cpp" "platform/texture2d.h" 
 "platform/window.cpp" "platform/window.h" 
 "gui/gui_layer.cpp" "gui/gui_layer.h" 
 "gui/imconfig.h" "gui/imgui.cpp" "gui/imgui.h" "gui/imgui_demo.cpp" "gui/imgui_draw.cpp" "gui/imgui_internal.h" "gui/imgui_tables.cpp" "gui/imgui_widgets.cpp" "gui/imstb_rectpack.h" "gui/imstb_textedit.h" "gui/imstb_truetype.h" 
 "gui/backends/imgui_impl_glfw.cpp" "gui/backends/imgui_impl_glfw.h" "gui/backends/imgui_impl_opengl3.cpp" "gui/backends/imgui_impl_opengl3.h" "gui/backends/imgui_impl_opengl3_loader.h" 
 "renderer/renderer.cpp" "renderer/renderer.h"  
 "scene/scene.cpp" "scene/scene.h" 
 "optix/optix_renderer.h" "optix/optix_renderer.cpp" 
 "optix/check.h" 
 "optix/records.h" "core/pt_types.h" "camera/camera.h" "camera/camera.cpp" "optix/optix_scene.h" "optix/optix_scene.cpp" "util/onb.h" "util/texture.cpp" "util/texture.h" "util/constants.h" "util/image.h" "optix/load_obj.cpp")

 target_include_directories(BathTracerTwo
    PRIVATE
        ${OPTIX_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/cuda
)

target_link_libraries(BathTracerTwo 
    PRIVATE 
        CUDA::cuda_driver
        CUDA::cudart
        glm
        glfw
        glad)


if (WIN32)
    target_link_libraries(BathTracerTwo PRIVATE opengl32)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    target_link_libraries(BathTracerTwo PRIVATE
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${OPENGL_LIBRARY}
    )
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(BathTracerTwo PRIVATE OpenGL::GL)
endif()



if(WIN32)
    target_link_libraries(BathTracerTwo PRIVATE cfgmgr32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(BathTracerTwo PRIVATE dl)
endif()

# ptx gen
set(DEVICE_PTX "${CMAKE_CURRENT_BINARY_DIR}/device_programs.ptx")

add_custom_command(
    OUTPUT ${DEVICE_PTX}
    COMMAND ${CMAKE_CUDA_COMPILER}
            --ptx
            -std=c++17
            --use_fast_math
            -arch=sm_86
            -I"${OPTIX_INCLUDE_DIR}"
            -I"${CMAKE_SOURCE_DIR}/external"
            -I"${CMAKE_SOURCE_DIR}/src"
            -I"${CMAKE_SOURCE_DIR}/cuda"
            "${CMAKE_SOURCE_DIR}/cuda/device_programs.cu"
            -o "${DEVICE_PTX}"
    DEPENDS "${CMAKE_SOURCE_DIR}/cuda/device_programs.cu"
    COMMENT "Compiling device_programs.cu to device_programs.ptx"
)

add_custom_target(device_ptx ALL DEPENDS ${DEVICE_PTX})
add_dependencies(BathTracerTwo device_ptx)

target_compile_definitions(BathTracerTwo PRIVATE
    DEVICE_PTX_PATH="${DEVICE_PTX}"
)

if (MSVC)
    target_compile_options(BathTracerTwo PRIVATE /W4 /permissive- /EHsc)
    target_compile_definitions(BathTracerTwo PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(BathTracerTwo PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET BathTracerTwo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:BathTracerTwo>/assets
)