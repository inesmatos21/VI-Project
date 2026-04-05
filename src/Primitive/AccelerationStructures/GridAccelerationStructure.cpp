#include "Primitive/AccelerationStructures/GridAccelerationStructure.hpp"

#include "Ray/Intersection.hpp"
#include "Scene/Scene.hpp"

namespace VI
{

GridAccelerationStructure GridAccelerationStructure::Create(const Scene& scene [[maybe_unused]])
{
  return {};
}

bool GridAccelerationStructure::Trace(const Ray& ray [[maybe_unused]], const Scene& scene [[maybe_unused]], Intersection& intersection [[maybe_unused]]) const
{
  return false;
}

void GridAccelerationStructure::Build(const Scene& scene [[maybe_unused]]) {}

} // namespace VI
