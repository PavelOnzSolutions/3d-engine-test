#pragma once
#include <string>
#include <memory>

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
