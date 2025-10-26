#include "../Include/Systems.h"
#include "../Include/Components.h"

void UpdateMovement(entt::registry& registry, float deltaTime) {
    auto view = registry.view<Transform, Velocity>();
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        const auto& velocity = view.get<Velocity>(entity);
        transform.position += velocity.linear * deltaTime;
    }
}