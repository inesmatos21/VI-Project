#pragma once

#include "Math/RGB.hpp"
#include "Shaders/Shader.hpp"

namespace VI
{

struct Ray;
struct Intersection;
class Scene;
class Camera;
class Material;

class WhittedShader final
{
public:
  WhittedShader(const RGB& background_color) : m_BackgroundColor(background_color) {}

  RGB Execute(const Ray& ray, const Scene& scene) const;

private:
  RGB DoExecute(const Ray& ray, const Scene& scene, const Intersection& intersection, int depth = 0) const;

  RGB DirectIllumination(const Ray& ray, const Scene& scene, const Intersection& intersection, const Material& material) const;

  RGB IndirectIllumination(const Ray& ray, const Scene& scene, const Intersection& intersection, const Material& material, int depth) const;

  RGB m_BackgroundColor [[maybe_unused]];
};

static_assert(Shader<WhittedShader>);

} // namespace VI
