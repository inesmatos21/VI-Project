#pragma once

#include "Math/RGB.hpp"

namespace VI
{

class Light;
class Material;
class Scene;
struct Intersection;
struct Ray;

struct SelectedLight
{
  const Light* LightPtr{nullptr};
  float SelectionPDF{0.f};
};

enum class DirectIlluminationMode
{
  Uniform,
  Importance,
  All,
};

RGB EstimateDirectIllumination(const Ray& ray, const Scene& scene, const Intersection& intersection, const Material& material, const Light* selected_light);

RGB SampleDirectIllumination(const Ray& ray, const Scene& scene, const Intersection& intersection, const Material& material, DirectIlluminationMode mode = DirectIlluminationMode::Uniform);

} // namespace VI
