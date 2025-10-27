#pragma once

#include <string>
#include <vector>

namespace Engine { namespace Assets {

struct MeshData {
    struct VertexPNC {
        float px, py, pz;
        float nx, ny, nz;
        float u, v;
    };
    std::vector<VertexPNC> vertices;
    std::vector<uint32_t> indices; // 32-bit for simplicity; renderer may downcast to 16-bit if safe
};

// Load the first triangle-list primitive from a glTF/GLB file.
// POSITION is required; NORMAL/UV are optional and will be zeroed if missing.
// Returns true on success and fills out_mesh. On failure, returns false and fills out_warnings.
bool LoadFirstTriangleMesh(const std::string& scene_path,
                           MeshData& out_mesh,
                           std::vector<std::string>& out_warnings);

}} // namespace Engine::Assets
