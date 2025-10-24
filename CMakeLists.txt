cmake_minimum_required(VERSION 3.20)
project(3DEngineTest LANGUAGES CXX)

# Require MSVC toolchain to switch the project away from MinGW/g++ - TEMPORARY
if(NOT MSVC)
    message(FATAL_ERROR "This project is configured to use the MSVC toolchain. Please switch your CLion profile/toolchain to MSVC (Visual Studio) and re-configure.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENGINE_WITH_DX12 "Enable DirectX 12 renderer backend" ON)
option(ENGINE_WITH_VULKAN "Enable Vulkan renderer backend" ON)
option(USE_METHANEKIT "Fetch and integrate MethaneKit (optional)" ON)

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
        engine/renderer/Src/RendererFactory.cpp)
target_link_libraries(engine_renderer_factory PUBLIC engine_renderer_abstraction)

# Assets, Threading, Networking (headers-only for now)
add_library(engine_assets INTERFACE)
target_include_directories(engine_assets
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/engine/assets/Include)

add_library(engine_threading INTERFACE)
target_include_directories(engine_threading
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/engine/threading/Include)
if(Boost_THREAD_FOUND OR TARGET Boost::thread)
    target_link_libraries(engine_threading INTERFACE Boost::thread)
endif()

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
        GIT_TAG v0.8.1
    )
    FetchContent_MakeAvailable(MethaneKit)
    # Link your renderer implementations with MethaneKit targets when enabling real integration.
endif()

# App executable showing an empty window
add_executable(3DEngineTest WIN32
        main.cpp)

target_link_libraries(3DEngineTest PRIVATE
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
