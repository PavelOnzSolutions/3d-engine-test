#pragma once
#include <string>
#include <memory>
#include <vector>

namespace Engine::Assets {

// Placeholder resource types used by the ResourceManager. These can be
// replaced with real backend-specific resources later.
struct Texture
{
    std::string id;   // key or path
    std::string path; // original file path
};

struct Mesh
{
    std::string id;
    std::string path;

    // Minimal geometry containers for meshes. These can be filled by loaders
    // (e.g., glTF) and then consumed by renderer backends.
    std::vector<float>    positions; // x,y,z triples
    std::vector<float>    normals;   // x,y,z triples
    std::vector<float>    texcoords; // u,v pairs
    std::vector<uint32_t> indices;   // triangle indices

    // Metadata
    size_t primitive_count = 0;
    bool   is_gltf = false;
};

struct Shader
{
    std::string id;
    std::string path;
    std::string entry_point; // optional metadata
    std::string stage;       // e.g., "vs", "ps", "cs" (backend-agnostic placeholder)
};

using TexturePtr = std::shared_ptr<Texture>;
using MeshPtr    = std::shared_ptr<Mesh>;
using ShaderPtr  = std::shared_ptr<Shader>;

} // namespace Engine::Assets
