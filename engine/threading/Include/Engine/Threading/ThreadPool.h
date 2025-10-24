#pragma once
#include <functional>
#include <vector>

namespace Engine::Threading {

class ThreadPool {
public:
    void Enqueue(std::function<void()> /*task*/) {}
};

} // namespace Engine::Threading
