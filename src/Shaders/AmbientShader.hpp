#pragma once

#include "Math/RGB.hpp"
#include "Shaders/Shader.hpp"

namespace VI
{

class Scene;
class Camera;
struct Ray;

class AmbientShader final
{
public:
  AmbientShader(const RGB& background_color) : m_BackgroundColor(background_color) {}

  RGB Execute(const Ray& ray, const Scene& scene) const;

private:
  RGB m_BackgroundColor [[maybe_unused]];
};

static_assert(Shader<AmbientShader>);

} // namespace VI
