cmake_minimum_required(VERSION 3.20)
project(3DEngineTest LANGUAGES CXX)

# Require MSVC toolchain to switch the project away from MinGW/g++ - TEMPORARY
if(NOT MSVC)
    message(FATAL_ERROR "This project is configured to use the MSVC toolchain. Please switch your CLion profile/toolchain to MSVC (Visual Studio) and re-configure.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Workaround for external dependency warning treated as error in fmt when building MethaneKit
# Suppress MSVC warning C4459 (declaration hides global) globally so external projects don't fail with /WX
if(MSVC)
    add_compile_options(/wd4459)
endif()

option(ENGINE_WITH_DX12 "Enable DirectX 12 renderer backend" ON)
option(ENGINE_WITH_VULKAN "Enable Vulkan renderer backend" ON)
option(USE_METHANEKIT "Fetch and integrate MethaneKit (optional)" OFF)
# Disable MethaneKit by default; use pure Direct3D 12 path for Windows builds
set(USE_METHANEKIT OFF CACHE BOOL "Fetch and integrate MethaneKit (optional)" FORCE)

# Guard: MethaneKit currently requires MSVC + Windows SDK. Disable on other toolchains (e.g., MinGW)  - TEMPORARY
if(USE_METHANEKIT AND NOT MSVC)
    message(WARNING "USE_METHANEKIT=ON requested, but this toolchain is not MSVC. Disabling MethaneKit to avoid SDK/pkg-config errors.")
    set(USE_METHANEKIT OFF CACHE BOOL "Fetch and integrate MethaneKit (optional)" FORCE)
endif()

# Optional Boost for threading and networking abstractions
find_package(Boost QUIET COMPONENTS thread system)

# Core engine library
add_library(engine_core STATIC
        engine/core/Src/Engine.cpp
        engine/core/Src/Config.cpp
        engine/core/Src/Logger.cpp)
target_include_directories(engine_core
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/core/Include)

# Renderer abstraction + factory
add_library(engine_renderer_abstraction INTERFACE)
target_include_directories(engine_renderer_abstraction
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/engine/renderer/Include)

add_library(engine_renderer_factory STATIC
        engine/renderer/Src/RendererFactory.cpp
        engine/renderer/Src/Core/FrameGraph.cpp
        engine/renderer/Src/RenderContextMethane.cpp)
# Link only abstraction by default; add Methane RHI when enabled
if(WIN32)
    target_link_libraries(engine_renderer_factory PUBLIC engine_renderer_abstraction)
else()
    target_link_libraries(engine_renderer_factory PUBLIC engine_renderer_abstraction)
endif()

# Link D3D12 system libraries for the Direct3D path in the factory
if(WIN32)
    target_link_libraries(engine_renderer_factory PUBLIC d3d12 dxgi d3dcompiler engine_assets)
endif()

# When MethaneKit is enabled, link RHI targets to allow creating a render context and presenting frames
if(USE_METHANEKIT)
    target_link_libraries(engine_renderer_factory PUBLIC
        MethaneGraphicsRhiInterface
        MethaneGraphicsRhiDirectX
        MethanePlatformAppView
        Taskflow
    )
endif()

# Assets, Threading, Networking (headers-only for now)
add_library(engine_assets STATIC
        engine/assets/Src/SceneLoader.cpp
        engine/assets/Src/GlbLoader.cpp)

# Fetch cgltf (official single-header glTF 2.0 library)
include(FetchContent)
FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG v1.13
)
FetchContent_MakeAvailable(cgltf)

# Expose engine assets includes and cgltf include directory
 target_include_directories(engine_assets
         PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/assets/Include
         PRIVATE ${cgltf_SOURCE_DIR})
 
 target_link_libraries(engine_assets PUBLIC engine_core)

add_library(engine_threading INTERFACE)
target_include_directories(engine_threading
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/engine/threading/Include)
if(Boost_THREAD_FOUND OR TARGET Boost::thread)
    target_link_libraries(engine_threading INTERFACE Boost::thread)
endif()

add_library(engine_sound INTERFACE)
target_include_directories(engine_sound INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/engine/sound/Include)

add_library(engine_networking INTERFACE)
target_include_directories(engine_networking
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/engine/networking/Include)
if(Boost_SYSTEM_FOUND OR TARGET Boost::system)
    target_link_libraries(engine_networking INTERFACE Boost::system)
endif()

# Renderer backends (stubs)
if(ENGINE_WITH_DX12)
    add_library(engine_renderer_dx12 STATIC
            engine/renderer_dx12/Src/RendererDX12.cpp)
    target_link_libraries(engine_renderer_dx12 PUBLIC engine_renderer_abstraction)
endif()

if(ENGINE_WITH_VULKAN)
    add_library(engine_renderer_vulkan STATIC
            engine/renderer_vulkan/Src/RendererVulkan.cpp)
    target_link_libraries(engine_renderer_vulkan PUBLIC engine_renderer_abstraction)
endif()

# Optional MethaneKit (scaffold only; OFF by default to avoid heavy deps)
if(USE_METHANEKIT)
    include(FetchContent)
    FetchContent_Declare(
        MethaneKit
        GIT_REPOSITORY https://github.com/MethanePowered/MethaneKit.git
        GIT_TAG 197855c
    )
    FetchContent_MakeAvailable(MethaneKit)
    # Define a project-wide macro to enable Methane-backed wrappers in headers
    target_compile_definitions(engine_renderer_abstraction INTERFACE ENGINE_USE_METHANEKIT)
    # Link your renderer implementations with MethaneKit targets when enabling real integration.
endif()

include(FetchContent)

# Fetch GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1   # Replace with latest stable tag
)

FetchContent_MakeAvailable(glm)

# EnTT ECS library via FetchContent (always available)
include(FetchContent)
FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.13.0 # or latest stable
)
FetchContent_MakeAvailable(entt)

include(FetchContent)
# --- Fetch OpenAL Soft ---
FetchContent_Declare(
        openal-soft
        GIT_REPOSITORY https://github.com/kcat/openal-soft.git
        GIT_TAG master   # or specify a stable release tag like "1.23.1"
)
# Optionally customize OpenAL build options before making it available
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(openal-soft)


add_executable(3DEngineTest WIN32
        main.cpp
        engine/ecs/Include/Components.h
        engine/ecs/Include/Systems.h
        engine/ecs/Src/Systems.cpp
        toolbox/SceneGen/main.cpp
)

target_link_libraries(3DEngineTest PRIVATE
        EnTT::EnTT
        glm::glm
        OpenAL::OpenAL
        engine_core
        engine_renderer_abstraction
        engine_renderer_factory
        engine_assets
        engine_threading
        engine_networking
)

# Link Win32 libs for window creation with MinGW/MSVC
if(WIN32)
    target_link_libraries(3DEngineTest PRIVATE user32 gdi32)
endif()

# On MinGW, statically link GCC, libstdc++, and winpthread to avoid missing runtime DLLs at startup
if(MINGW)
    target_link_options(3DEngineTest PRIVATE 
        -static-libgcc 
        -static-libstdc++ 
        -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic)
endif()
