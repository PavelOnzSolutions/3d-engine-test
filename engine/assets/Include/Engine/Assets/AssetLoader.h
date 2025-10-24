#pragma once
#include <string>

namespace Engine::Assets {

class AssetLoader {
public:
    bool LoadModel(const std::string& /*path*/) { return true; }
    bool LoadTexture(const std::string& /*path*/) { return true; }
    bool LoadSound(const std::string& /*path*/) { return true; }
};

} // namespace Engine::Assets
