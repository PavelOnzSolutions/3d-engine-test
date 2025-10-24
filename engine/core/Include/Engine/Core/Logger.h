#pragma once
#include <string>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <chrono>

namespace Engine::Core {

struct LoggingConfig {
    // Maximum size of the active log file before rotation, in megabytes.
    // Default: 5 MB
    int max_file_size_mb = 5;
    // Maximum number of rotated log files to keep (not counting the active file).
    // Default: 5 files
    int max_file_count = 5;
};

class Logger {
public:
    // Initialize logger with configuration. Safe to call multiple times; later calls update settings.
    static void Initialize(const LoggingConfig& cfg);

    // Writes a line to the log with timestamp. Thread-safe.
    static void Log(const std::string& message);

    // Convenience overloads
    static void Info(const std::string& message) { Log(message); }
    static void Error(const std::string& message) { Log(std::string("ERROR: ") + message); }

private:
    Logger();
    ~Logger();

    static Logger& Instance();

    void set_config(const LoggingConfig& cfg);
    void write_line(const std::string& message);
    void ensure_open();
    void rotate_if_needed(size_t incoming_bytes);
    void rotate();
    void enforce_file_count_limit();

    static std::string GetTimestampForFile();
    static std::string GetTimestampForLine();

private:
    std::mutex m_mutex;
    std::ofstream m_stream;
    std::filesystem::path m_log_path;          // 3DEngineTest.log
    std::filesystem::path m_log_dir;           // working directory
    LoggingConfig m_cfg;
};

} // namespace Engine::Core
