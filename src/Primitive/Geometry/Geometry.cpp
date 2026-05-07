#include "Primitive/Geometry/Geometry.hpp"

#include <variant>

#include "Primitive/BoundingBox.hpp"

namespace VI
{
bool Intersect(const Geometry& geom, const Ray& ray, Intersection& intersection)
{
  return std::visit([&](const auto& shape) { return shape.Intersect(ray, intersection); }, geom);
}

BoundingBox GetBoundingBox(const Geometry& geom)
{
  return std::visit([&](const auto& shape) { return shape.GetBoundingBox(); }, geom);
}
} // namespace VI
