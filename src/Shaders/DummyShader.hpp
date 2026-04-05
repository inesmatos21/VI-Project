#pragma once

#include "Math/RGB.hpp"
#include "Scene/Scene.hpp"
#include "Shaders/Shader.hpp"

namespace VI
{
struct Ray;

class DummyShader final
{
public:
  DummyShader(int width, int height) : m_Width{width}, m_Height{height} {}

  RGB Execute(const Ray& ray, const Scene& scene [[maybe_unused]]) const
  {
    float width = static_cast<float>(m_Width);
    float height = static_cast<float>(m_Height);

    return {ray.Direction.x / width, ray.Direction.y / height, 0.f};
  }

private:
  int m_Width, m_Height;
};

static_assert(Shader<DummyShader>);
} // namespace VI
