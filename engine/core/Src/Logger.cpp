#include <Engine/Core/Logger.h>

#include <iomanip>
#include <sstream>
#include <vector>

namespace Engine::Core {

static constexpr const char* kActiveLogName = "3DEngineTest.log";

static uintmax_t FileSizeSafe(const std::filesystem::path& p)
{
    std::error_code ec;
    auto sz = std::filesystem::file_size(p, ec);
    return ec ? 0u : sz;
}

Logger::Logger()
{
    m_log_dir = std::filesystem::current_path();
    m_log_path = m_log_dir / kActiveLogName;
}

Logger::~Logger()
{
    if (m_stream.is_open())
        m_stream.close();
}

Logger& Logger::Instance()
{
    static Logger s_instance;
    return s_instance;
}

void Logger::Initialize(const LoggingConfig& cfg)
{
    Instance().set_config(cfg);
}

void Logger::set_config(const LoggingConfig& cfg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cfg = cfg;
    ensure_open();
}

void Logger::ensure_open()
{
    if (m_stream.is_open())
        return;

    std::error_code ec;
    std::filesystem::create_directories(m_log_dir, ec);

    m_stream.open(m_log_path, std::ios::out | std::ios::app | std::ios::binary);
}

void Logger::Log(const std::string& message)
{
    Instance().write_line(message);
}

std::string Logger::GetTimestampForFile()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &t);
#else
    localtime_r(&t, &local_tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

std::string Logger::GetTimestampForLine()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &t);
#else
    localtime_r(&t, &local_tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::write_line(const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ensure_open();

    std::string line;
    line.reserve(message.size() + 64);
    line.append("[");
    line.append(GetTimestampForLine());
    line.append("] ");
    line.append(message);
    line.append("\n");

    rotate_if_needed(line.size());

    if (m_stream.is_open())
    {
        m_stream.write(line.data(), static_cast<std::streamsize>(line.size()));
        m_stream.flush();
    }
}

void Logger::rotate_if_needed(size_t incoming_bytes)
{
    const uintmax_t max_bytes = static_cast<uintmax_t>(std::max(1, m_cfg.max_file_size_mb)) * 1024ull * 1024ull;
    const uintmax_t current_size = FileSizeSafe(m_log_path);
    if (current_size + incoming_bytes > max_bytes)
    {
        rotate();
    }
}

void Logger::rotate()
{
    if (m_stream.is_open())
    {
        m_stream.flush();
        m_stream.close();
    }

    std::string ts = GetTimestampForFile();
    std::filesystem::path rotated = m_log_dir / (std::string("3DEngineTest_") + ts + ".log");

    std::error_code ec;
    std::filesystem::rename(m_log_path, rotated, ec);
    if (ec)
    {
        // If rename fails (e.g., file does not exist), attempt to remove target and try again.
        std::filesystem::remove(rotated, ec);
        std::filesystem::rename(m_log_path, rotated, ec);
    }

    // Reopen active log
    m_stream.open(m_log_path, std::ios::out | std::ios::app | std::ios::binary);

    enforce_file_count_limit();
}

void Logger::enforce_file_count_limit()
{
    // Gather rotated files matching pattern 3DEngineTest_*.log in the same directory
    std::vector<std::filesystem::directory_entry> entries;
    std::error_code ec;
    for (auto& de : std::filesystem::directory_iterator(m_log_dir, ec))
    {
        if (ec) break;
        if (!de.is_regular_file()) continue;
        const auto& p = de.path();
        const auto fname = p.filename().string();
        if (fname.rfind("3DEngineTest_", 0) == 0 && p.extension() == ".log")
        {
            entries.emplace_back(de);
        }
    }

    // Sort by last_write_time (oldest first)
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b){
        std::error_code e1, e2;
        auto ta = std::filesystem::last_write_time(a, e1);
        auto tb = std::filesystem::last_write_time(b, e2);
        if (e1 || e2) return a.path() < b.path();
        return ta < tb;
    });

    int max_count = std::max(1, m_cfg.max_file_count);
    if (static_cast<int>(entries.size()) > max_count)
    {
        int to_delete = static_cast<int>(entries.size()) - max_count;
        for (int i = 0; i < to_delete; ++i)
        {
            std::filesystem::remove(entries[i], ec);
        }
    }
}

} // namespace Engine::Core
