#pragma once

#include "Math/RGB.hpp"

#include <concepts>

namespace VI {
class Camera;
class Scene;
struct Ray;

template <class T>
concept Shader = requires(const T &shader, const Ray &ray, const Scene &scene) {
  { shader.Execute(ray, scene) } -> std::same_as<RGB>;
};

} // namespace VI
