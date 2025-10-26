#pragma once
#include <glm/glm.hpp>
#include <entt/entt.hpp>

// Basic spatial transform
struct Transform {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

// Linear velocity for simple movement demo
struct Velocity {
    glm::vec3 linear{0.0f};
};

// Mesh renderer binding: connects entity to mesh & material indices resolved from glTF
struct MeshRenderer {
    int mesh_id{-1};
    int material_id{-1};
};

// Scene hierarchy: parent reference; entt::null if root
struct Parent {
    entt::entity value{ entt::null };
};