#pragma once
#include <string>

namespace Engine::Core {

struct LoggingConfig; // forward declaration

struct VideoConfig {
    bool fullscreen = false;
    std::string renderer = "direct3d"; // allowed: direct3d|vulkan (also accept synonyms)
    int width = 1280;
    int height = 720;
};

// Engine assets directories configuration
struct EngineConfig {
    std::string dirModels;   // e.g., path to models root
    std::string dirTextures; // e.g., path to textures root
    std::string dirSounds;   // e.g., path to sounds root
    std::string dirScenes;   // e.g., path to scenes root
};

class Config {
public:
    // Loads configuration from "3DEngineTest.ini" in the working directory.
    // Returns true if file exists and at least one value was parsed; false otherwise.
    static bool LoadFromIni(VideoConfig& outVideo);

    // Loads logging configuration from INI [Logging] section.
    // Returns true if file exists and at least one logging value was parsed; false otherwise.
    static bool LoadLoggingFromIni(LoggingConfig& outLogging);

    // Loads engine assets directories from INI [Engine] section.
    // Returns true if file exists and at least one engine value was parsed; false otherwise.
    static bool LoadEngineFromIni(EngineConfig& outEngine);
};

} // namespace Engine::Core
