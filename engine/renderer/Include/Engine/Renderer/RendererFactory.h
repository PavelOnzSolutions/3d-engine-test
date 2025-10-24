#pragma once
#include <memory>
#include <string>
#include <Engine/Renderer/IRenderer.h>

namespace Engine::Renderer {

// Creates a renderer by ID. Supported ids: "direct3d", "vulkan", "methane".
// Returns nullptr if id is unknown.
std::unique_ptr<IRenderer> CreateRenderer(const std::string& id);

} // namespace Engine::Renderer
