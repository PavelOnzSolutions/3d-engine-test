#pragma once
#include <string>

namespace Engine::Networking {

class Network {
public:
    bool Connect(const std::string& /*host*/, int /*port*/) { return true; }
};

} // namespace Engine::Networking
