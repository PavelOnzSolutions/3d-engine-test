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

class Config {
public:
    // Loads configuration from "3DEngineTest.ini" in the working directory.
    // Returns true if file exists and at least one value was parsed; false otherwise.
    static bool LoadFromIni(VideoConfig& outVideo);

    // Loads logging configuration from INI [Logging] section.
    // Returns true if file exists and at least one logging value was parsed; false otherwise.
    static bool LoadLoggingFromIni(LoggingConfig& outLogging);
};

} // namespace Engine::Core
