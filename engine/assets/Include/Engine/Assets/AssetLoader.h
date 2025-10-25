#pragma once
#include <string>
#include <algorithm>
#include <cctype>

#include <Engine/Assets/ResourceTypes.h>

namespace Engine::Assets {

class AssetLoader {
public:
    // Legacy stub loaders
    bool LoadModel(const std::string& /*path*/) { return true; }
    bool LoadTexture(const std::string& /*path*/) { return true; }
    bool LoadSound(const std::string& /*path*/) { return true; }

    // Minimal glTF mesh loader stub: detects .gltf/.glb and fills Mesh metadata.
    // This keeps the code header-only and avoids heavy third-party deps for now.
    bool LoadGltfMesh(const std::string& path, Mesh& out_mesh)
    {
        auto ext = GetLowerExtension(path);
        if (ext != ".gltf" && ext != ".glb")
            return false;

        // For now, we do not parse the file; we only mark metadata so higher
        // levels can treat this mesh as glTF-based. Real parsing can be added later.
        out_mesh.path = path;
        out_mesh.is_gltf = true;
        out_mesh.primitive_count = 0; // unknown without parsing
        return true;
    }

private:
    static std::string GetLowerExtension(const std::string& path)
    {
        auto pos = path.find_last_of('.');
        if (pos == std::string::npos)
            return {};
        std::string ext = path.substr(pos);
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return ext;
    }
};

} // namespace Engine::Assets
