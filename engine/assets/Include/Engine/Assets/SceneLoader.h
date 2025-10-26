#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace Engine { namespace Core { struct EngineConfig; }}

namespace Engine::Assets {

struct LoadedScene {
    std::string scene_path;       // resolved absolute or relative path
    int nodes_count = 0;          // number of nodes parsed (stubbed)
    int meshes_count = 0;         // stub
    int materials_count = 0;      // stub
    int textures_count = 0;       // stub
};

// Runtime scene loader facade. In future, implement using tinygltf or cgltf to parse .glb/.gltf
// For now, this is a stub that validates paths and pretends to load a scene.
bool LoadScene(const Engine::Core::EngineConfig& cfg,
               const std::string& startup_scene_file,
               LoadedScene& out_scene,
               std::vector<std::string>& out_warnings);

} // namespace Engine::Assets
