#pragma once

#include "Primitive/AccelerationStructures/AccelerationStructure.hpp"
#include "Primitive/AccelerationStructures/BVH.hpp"

namespace VI
{

class BVHAccelerationStructure final : public AccelerationStructure
{
public:
  static BVHAccelerationStructure Create(const Scene& scene);
  bool Trace(const Ray& ray, const Scene& scene, Intersection& intersection) const override;
  void Build(const Scene& scene) override;

private:
  BVH m_BVH{};
};

} // namespace VI
