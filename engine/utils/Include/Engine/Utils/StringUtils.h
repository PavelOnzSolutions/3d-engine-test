#pragma once
#include <string>
#include <windows.h>

namespace Engine::Utils {
    class StringUtils {
    public:
        static std::wstring Utf8ToWide(const std::string& utf8);
    };
}