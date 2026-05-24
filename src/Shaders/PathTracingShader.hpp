#pragma once

#include "Math/RGB.hpp"
#include "Shaders/DirectIllumination.hpp"
#include "Shaders/Shader.hpp"

namespace VI
{
struct Ray;
struct Intersection;
class Material;
class Scene;

class PathTracingShader final
{
public:
  explicit PathTracingShader(const RGB& background_color = {}, DirectIlluminationMode direct_illumination_mode = DirectIlluminationMode::Uniform) : m_BackgroundColor{background_color}, m_DirectIlluminationMode{direct_illumination_mode} {}

  RGB Execute(const Ray& ray, const Scene& scene) const;

private:
  RGB DoExecute(const Ray& ray, const Scene& scene, const Intersection& intersection, int depth = 0, bool allow_emissive = true) const;

  RGB DirectIllumination(const Ray& ray, const Scene& scene, const Intersection& intersection) const;

  RGB IndirectIllumination(const Ray& ray, const Scene& scene, const Intersection& intersection, const Material& material, int depth, bool allow_emissive [[maybe_unused]]) const;

  RGB DielectricScatter(const Ray& ray, const Scene& scene, const Intersection& Intersection, const Material& material, int depth) const;

  RGB m_BackgroundColor;
  DirectIlluminationMode m_DirectIlluminationMode;
};

static_assert(Shader<PathTracingShader>);
} // namespace VI
