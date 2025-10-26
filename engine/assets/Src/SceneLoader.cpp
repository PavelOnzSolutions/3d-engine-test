#include <Engine/Assets/SceneLoader.h>
#include <Engine/Core/Config.h>
#include <Engine/Core/Logger.h>
#include <fstream>

namespace fs = std::filesystem;

namespace Engine::Assets {

static bool IsGlTFPath(const fs::path& p)
{
    auto ext = p.extension().string();
    for (auto& c : ext) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    return (ext == ".gltf" || ext == ".glb");
}

bool LoadScene(const Engine::Core::EngineConfig& cfg,
               const std::string& startup_scene_file,
               LoadedScene& out_scene,
               std::vector<std::string>& out_warnings)
{
    using Engine::Core::Logger;

    out_scene = LoadedScene{};
    out_warnings.clear();

    fs::path candidate = startup_scene_file;
    if (!candidate.has_extension())
    {
        // Default to .glb if no extension provided
        candidate += ".glb";
    }

    // Resolve against dirScenes if provided
    if (!cfg.dirScenes.empty())
    {
        fs::path p = fs::path(cfg.dirScenes) / candidate;
        candidate = p;
    }

    // Normalize
    {
        std::error_code ec2;
        candidate = fs::weakly_canonical(candidate, ec2);
    }

    if (!IsGlTFPath(candidate))
    {
        out_warnings.emplace_back("Startup scene does not point to a glTF file (.glb/.gltf): " + candidate.string());
    }

    // For stub: check if file exists; if not, warn but continue with an empty scene
    std::error_code ec;
    bool exists = fs::exists(candidate, ec);
    if (!exists)
    {
        out_warnings.emplace_back("Scene file not found: " + candidate.string());
        Logger::Info("[SceneLoader] Scene file not found, continuing with empty scene");
        out_scene.scene_path = candidate.string();
        out_scene.nodes_count = 0;
        return true; // not a hard error for stub
    }

    // Read a bit to simulate parsing and count nodes (stub behavior):
    // we'll count occurrences of '"nodes"' token as heuristic
    int nodes = 0;
    try
    {
        std::ifstream ifs(candidate, std::ios::binary);
        if (ifs)
        {
            std::string contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            std::string needle = "\"nodes\"";
            size_t pos = 0;
            while ((pos = contents.find(needle, pos)) != std::string::npos)
            {
                ++nodes; pos += needle.size();
            }
        }
    }
    catch (...) { /* ignore */ }

    // Minimal fake counts
    out_scene.scene_path = candidate.string();
    out_scene.nodes_count = std::max(1, nodes); // ensure at least 1 node exists for demo
    out_scene.meshes_count = std::max(0, out_scene.nodes_count - 1);
    out_scene.materials_count = out_scene.meshes_count;
    out_scene.textures_count = out_scene.meshes_count;

    Logger::Info(std::string("[SceneLoader] Loaded scene stub: ") + out_scene.scene_path +
                 ", nodes=" + std::to_string(out_scene.nodes_count) +
                 ", meshes=" + std::to_string(out_scene.meshes_count));

    return true;
}

} // namespace Engine::Assets
