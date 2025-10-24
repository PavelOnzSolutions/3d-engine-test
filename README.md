# 3D Game Engine Test (Proof of Concept)

A small 3D game engine proof of concept built for learning purposes. The goal of this repository is to experiment with a minimal engine architecture on Windows, try multiple rendering backends (Direct3D 12, Vulkan), and practice common engine subsystems like configuration and logging.

This is not a production-ready engine. It is a sandbox for exploring concepts and libraries, and for iterating on ideas.

---

## Highlights
- Win32 desktop application entry point (no external framework for windowing)
- Pluggable renderer backends via a simple factory interface
  - Direct3D 12 backend (default)
  - Vulkan backend (experimental)
- Configurable behavior via INI file (3DEngineTest.ini)
  - Renderer selection, resolution, fullscreen
  - Logging parameters
- Simple logger with rolling behavior (max file size / count)
- Iterative CMake build that pulls in third-party libraries required by the backends
- RendererCore: minimal FrameGraph + IRenderPass abstraction at engine level
- Example passes: GBuffer, Lighting, PostProcess, Present
- Integrated into DX12/Vulkan/Methane stubs; graph compiles once and executes each frame within a RenderContext begin/end scope

---

## Short Summary
- Added a header-only ResourceManager with thread-safe caching for textures, meshes, and shaders; uses AssetLoader on cache misses. No CMake changes required.

## Repository Layout
- main.cpp – WinMain, creates window, loads config, sets up logging, creates and drives the renderer
- engine/
  - core/ – configuration and logging subsystems
  - renderer/ – renderer interface and factory
  - renderer_dx12/ – Direct3D 12 implementation (backend)
  - renderer_vulkan/ – Vulkan implementation (backend)
  - networking/, threading/, assets/ – placeholders or early experiments for future work
- CMakeLists.txt – top-level build; external dependencies are managed as part of the build
- 3DEngineTest.ini – runtime configuration

---

## Building
The project is configured for CLion with MSVC toolchains on Windows and provides two CMake profiles:
- Debug-Visual Studio
- Release-Visual Studio

Recommended steps in CLion:
1. Open the project folder.
2. Select one of the existing CMake profiles (Debug-Visual Studio or Release-Visual Studio).
3. Build the desired target (see Targets section below). The primary executable target is 3DEngineTest.

Command-line (from CLion’s terminal using configured CMake profile):
- Debug build and run tests (example):
  - cmake --build cmake-build-debug-visual-studio --target 3DEngineTest
- Release build:
  - cmake --build cmake-build-release-visual-studio --target 3DEngineTest

Note: External dependencies (e.g., DirectX-related, MethaneKit subcomponents, fmt, FreeType, etc.) are pulled and built as part of the CMake process. The first build may take a while.

### Available top-level targets (selection)
- 3DEngineTest (executable)
- engine_core (library)
- engine_renderer_factory (library)
- engine_renderer_dx12 (library)
- engine_renderer_vulkan (library)
- nowide, fmt, freetype, and several Methane* libraries (third-party)

---

## Running
Run the 3DEngineTest executable produced by your chosen profile. The application:
- Loads settings from 3DEngineTest.ini
- Initializes logging
- Creates a Win32 window with a title reflecting the chosen renderer
- Initializes the selected renderer backend
- Enters a simple message loop and calls RenderFrame on each iteration

Close the window to exit. Logs will include initialization and shutdown details.

---

## Configuration (3DEngineTest.ini)
A sample 3DEngineTest.ini is provided at the repository root. On startup, defaults are applied and then overridden by values present in the INI.

Key sections/fields:
- [Video]
  - renderer: string, one of direct3d or vulkan (unknown values fall back to direct3d)
  - fullscreen: true/false
  - width: integer (windowed mode)
  - height: integer (windowed mode)
- [Logging]
  - max_file_size_mb: integer
  - max_file_count: integer

The main program logs a summary of loaded configuration values at startup.

---

## Dependencies and Credits
The build integrates third-party components via CMake. Notable entries visible in the generated targets include:
- Microsoft DirectX headers, DirectXTex, and DirectX Shader Compiler (for D3D12 backend)
- Vulkan-related support (via engine_renderer_vulkan; exact loader/runtime assumed available on the system)
- fmt (formatting)
- FreeType (fonts; used by UI/typography samples within MethaneKit dependencies)
- MethaneKit modules (graphics primitives, UI assets and shaders, etc.)
- Boost.Nowide, CLI11, and other utilities pulled in transitively as needed

Many of these are included for experimentation and may evolve as the PoC changes.

---

## Platform Requirements
- Windows 10/11, x64
- MSVC toolchain (configured by CLion profile)
- GPU/driver supporting the selected backend:
  - Direct3D 12 for the direct3d renderer
  - Vulkan 1.x runtime/driver for the vulkan renderer

---

## Current Status and Roadmap
Status: Basic window creation, configuration, logging, and renderer backend scaffolding are in place. Rendering loop calls into backend each frame.

Possible next steps (learning-oriented):
- Input handling and basic camera controls
- Swap chain and frame pacing improvements
- Mesh/texture loading and a minimal scene graph
- Basic materials/shaders for both backends
- UI overlay with debug stats
- Job system integration (engine/threading) and frame tasks
- Networking experiments (engine/networking)
- Asset pipeline and hot-reload experiments
- Automated tests for core subsystems

---

## Troubleshooting
- Build takes a long time or fails while fetching dependencies:
  - Ensure internet access for first configure/build.
  - Clean CMake cache if dependencies changed significantly.
- Renderer fails to initialize:
  - Check GPU/driver support for the chosen backend.
  - Try switching renderer in 3DEngineTest.ini to direct3d.
- Window does not appear on secondary monitor in fullscreen:
  - Current PoC uses primary display metrics; multi-monitor handling is a future improvement.

