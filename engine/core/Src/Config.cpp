#include <Engine/Core/Config.h>
#include <Engine/Core/Logger.h>

#include <fstream>
#include <sstream>
#include <algorithm>

namespace Engine::Core {

static std::string Trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool StartsWith(const std::string& s, const std::string& prefix)
{
    return s.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), s.begin());
}

static bool ParseBool(const std::string& v, bool& out)
{
    std::string l = ToLower(Trim(v));
    if (l == "1" || l == "true" || l == "yes" || l == "on") { out = true; return true; }
    if (l == "0" || l == "false" || l == "no" || l == "off") { out = false; return true; }
    return false;
}

static std::string NormalizeRenderer(std::string s)
{
    s = ToLower(Trim(s));
    if (s == "direct3d" || s == "dx12" || s == "d3d12" || s == "d3d" || s == "directx") return "direct3d";
    if (s == "vulkan" || s == "vulcan" || s == "vk") return "vulkan"; // accept common misspelling
    if (s == "methane" || s == "methanekit" || s == "methane-kit" || s == "mk") return "methane";
    return s;
}

bool Config::LoadFromIni(VideoConfig& outVideo)
{
    std::ifstream file("3DEngineTest.ini");
    if (!file.is_open())
        return false;

    bool anyParsed = false;
    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        // Remove comments
        auto scPos = line.find(';');
        if (scPos != std::string::npos) line = line.substr(0, scPos);
        auto hashPos = line.find('#');
        if (hashPos != std::string::npos) line = line.substr(0, hashPos);

        line = Trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']')
        {
            currentSection = ToLower(Trim(line.substr(1, line.size() - 2)));
            continue;
        }

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = ToLower(Trim(line.substr(0, eqPos)));
        std::string val = Trim(line.substr(eqPos + 1));

        if (currentSection == "video")
        {
            if (key == "fullscreen")
            {
                bool b;
                if (ParseBool(val, b)) { outVideo.fullscreen = b; anyParsed = true; }
            }
            else if (key == "renderer")
            {
                outVideo.renderer = NormalizeRenderer(val);
                anyParsed = true;
            }
            else if (key == "width" || key == "w")
            {
                try { outVideo.width = std::max(1, std::stoi(val)); anyParsed = true; } catch (...) {}
            }
            else if (key == "height" || key == "h")
            {
                try { outVideo.height = std::max(1, std::stoi(val)); anyParsed = true; } catch (...) {}
            }
            else if (key == "resolution")
            {
                // support formats like 1920x1080 or 1920,1080
                std::string lower = ToLower(val);
                for (char& c : lower) { if (c == 'x' || c == 'X' || c == ',') c = 'x'; }
                auto xPos = lower.find('x');
                if (xPos != std::string::npos)
                {
                    try {
                        int w = std::stoi(Trim(lower.substr(0, xPos)));
                        int h = std::stoi(Trim(lower.substr(xPos + 1)));
                        if (w > 0 && h > 0) { outVideo.width = w; outVideo.height = h; anyParsed = true; }
                    } catch (...) {}
                }
            }
        }
    }

    return anyParsed;
}

bool Config::LoadLoggingFromIni(LoggingConfig& outLogging)
{
    std::ifstream file("3DEngineTest.ini");
    if (!file.is_open())
        return false;

    bool anyParsed = false;
    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        // Remove comments
        auto scPos = line.find(';');
        if (scPos != std::string::npos) line = line.substr(0, scPos);
        auto hashPos = line.find('#');
        if (hashPos != std::string::npos) line = line.substr(0, hashPos);

        line = Trim(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']')
        {
            currentSection = ToLower(Trim(line.substr(1, line.size() - 2)));
            continue;
        }

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = ToLower(Trim(line.substr(0, eqPos)));
        std::string val = Trim(line.substr(eqPos + 1));

        if (currentSection == "logging")
        {
            if (key == "maxfilesizemb" || key == "max_file_size_mb" || key == "max_size_mb")
            {
                try { outLogging.max_file_size_mb = std::max(1, std::stoi(val)); anyParsed = true; } catch (...) {}
            }
            else if (key == "maxfilecount" || key == "max_file_count" || key == "max_count")
            {
                try { outLogging.max_file_count = std::max(1, std::stoi(val)); anyParsed = true; } catch (...) {}
            }
        }
    }

    return anyParsed;
}

} // namespace Engine::Core
