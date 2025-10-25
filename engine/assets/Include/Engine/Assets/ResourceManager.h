#pragma once
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <utility>

#include <Engine/Assets/AssetLoader.h>
#include <Engine/Assets/ResourceTypes.h>

namespace Engine::Assets {

// A simple, header-only resource manager with basic caching for textures,
// meshes, and shaders. This is backend-agnostic and uses placeholder types
// defined in ResourceTypes.h. Integrate with real backends by replacing
// ResourceTypes with backend handles and enhancing AssetLoader.
class ResourceManager {
public:
    ResourceManager() = default;

    // Disable copy; allow move
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = default;
    ResourceManager& operator=(ResourceManager&&) = default;

    // Texture API
    TexturePtr GetTexture(const std::string& path)
    {
        const std::string key = NormalizeKey(path);
        if (auto tex = FindLocked(m_textures, key))
            return tex;

        std::unique_lock lock(m_mutex);
        auto it = m_textures.find(key);
        if (it != m_textures.end())
            return it->second;

        if (!m_loader.LoadTexture(path))
            return {};

        auto texture = std::make_shared<Texture>(Texture{ key, path });
        m_textures.emplace(key, texture);
        return texture;
    }

    bool UnloadTexture(const std::string& path)
    {
        const std::string key = NormalizeKey(path);
        std::unique_lock lock(m_mutex);
        return m_textures.erase(key) > 0;
    }

    // Mesh API
    MeshPtr GetMesh(const std::string& path)
    {
        const std::string key = NormalizeKey(path);
        if (auto mesh = FindLocked(m_meshes, key))
            return mesh;

        std::unique_lock lock(m_mutex);
        if (auto it = m_meshes.find(key); it != m_meshes.end())
            return it->second;

        // First try to load as glTF (.gltf/.glb)
        Mesh temp_mesh;
        if (m_loader.LoadGltfMesh(path, temp_mesh))
        {
            temp_mesh.id = key;
            auto mesh_ptr = std::make_shared<Mesh>(std::move(temp_mesh));
            m_meshes.emplace(key, mesh_ptr);
            return mesh_ptr;
        }

        // Fallback to legacy generic model loader
        if (!m_loader.LoadModel(path))
            return {};

        auto mesh_ptr = std::make_shared<Mesh>(Mesh{ key, path });
        m_meshes.emplace(key, mesh_ptr);
        return mesh_ptr;
    }

    bool UnloadMesh(const std::string& path)
    {
        const std::string key = NormalizeKey(path);
        std::unique_lock lock(m_mutex);
        return m_meshes.erase(key) > 0;
    }

    // Shader API
    ShaderPtr GetShader(const std::string& path,
                        std::string stage = {},
                        std::string entry_point = {})
    {
        const std::string key = MakeShaderKey(path, stage, entry_point);
        if (auto sh = FindLocked(m_shaders, key))
            return sh;

        std::unique_lock lock(m_mutex);
        auto it = m_shaders.find(key);
        if (it != m_shaders.end())
            return it->second;

        if (!m_loader.LoadTexture(path)) // using texture loader as a placeholder for I/O
            return {};

        auto shader = std::make_shared<Shader>(Shader{ key, path, std::move(entry_point), std::move(stage) });
        m_shaders.emplace(key, shader);
        return shader;
    }

    bool UnloadShader(const std::string& path,
                      const std::string& stage = {},
                      const std::string& entry_point = {})
    {
        const std::string key = MakeShaderKey(path, stage, entry_point);
        std::unique_lock lock(m_mutex);
        return m_shaders.erase(key) > 0;
    }

    // Clear all cached resources
    void Clear()
    {
        std::unique_lock lock(m_mutex);
        m_textures.clear();
        m_meshes.clear();
        m_shaders.clear();
    }

private:
    template<typename MapT>
    static auto FindLocked(const MapT& map, const std::string& key) -> typename MapT::mapped_type
    {
        std::shared_lock lock(m_mutex_global());
        auto it = map.find(key);
        if (it != map.end())
            return it->second;
        return {};
    }

    static std::string NormalizeKey(const std::string& s)
    {
        // For now, just return as-is. Could lowercase or canonicalize paths.
        return s;
    }

    static std::string MakeShaderKey(const std::string& path,
                                     const std::string& stage,
                                     const std::string& entry)
    {
        std::string key = NormalizeKey(path);
        key += "|";
        key += stage;
        key += "|";
        key += entry;
        return key;
    }

    // NOTE: We use a single mutex to guard all maps for simplicity.
    // Reads use a shared lock via a global accessor to satisfy template function requirements.
    static std::shared_mutex& m_mutex_global()
    {
        static std::shared_mutex g_mutex;
        return g_mutex;
    }

    std::shared_mutex& mutex() const { return m_mutex_global(); }

    // Members
    mutable std::shared_mutex m_mutex_instance_dummy; // unused, placeholder to satisfy potential future per-instance locking
    AssetLoader m_loader{};

    // Caches
    std::unordered_map<std::string, TexturePtr> m_textures;
    std::unordered_map<std::string, MeshPtr>    m_meshes;
    std::unordered_map<std::string, ShaderPtr>  m_shaders;

    // Convenience to use the same global mutex consistently
    static inline std::shared_mutex& m_mutex = m_mutex_global();
};

} // namespace Engine::Assets
