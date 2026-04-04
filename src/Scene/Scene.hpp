#pragma once

#include "Light/Light.hpp"
#include "Math/DiscreteDistribution.hpp"
#include "Primitive/AccelerationStructures/GridAccelerationStructure.hpp"
#include "Primitive/Geometry/Geometry.hpp"
#include "Primitive/Material.hpp"
#include "Primitive/Primitive.hpp"

#include <memory>
#include <span>

namespace VI {

struct Ray;
struct Intersection;

class Scene final {
public:
  void Build();

  bool Trace(const Ray &ray, Intersection &intersection) const;

  // Returns true if the path from ray origin to max_distance is unobstructed.
  bool Visibility(const Ray &ray, float max_distance) const;

  void AddPrimitive(Geometry primitive, int material_index);

  int AddMaterial(MaterialDesc &&desc);

  void AddLight(std::unique_ptr<Light> light);

  const Primitive &GetPrimitive(int primitive_index) const;

  const Material &GetMaterial(int material_index) const;

  std::span<const std::unique_ptr<Light>> GetLights() const;

  size_t GetPrimitiveCount() const;

  const LightSamplingDistribution &GetLightSamplingDistribution() const;

  BoundingBox ComputeBoundingBox() const;

private:
  std::vector<Primitive> m_Primitives{};
  std::vector<Material> m_Materials{};
  std::vector<std::unique_ptr<Light>> m_Lights{};
  LightSamplingDistribution m_LightSamplingDistribution{};
  GridAccelerationStructure m_AccelerationStructure{};
};
} // namespace VI
