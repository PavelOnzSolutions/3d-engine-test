#include <Engine/Utils/StringUtils.h>

namespace Engine::Utils {
    // Converts UTF-8 encoded string to widechar string
    std::wstring StringUtils::Utf8ToWide(const std::string& utf8) {
        if (utf8.empty()) return L"";
        int len = static_cast<int>(utf8.size());
        int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), len, nullptr, 0);
        if (wlen <= 0) return L"";
        std::wstring wstr(static_cast<size_t>(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), len, wstr.data(), wlen);
        return wstr;
    }
} // namespace Engine::Utils