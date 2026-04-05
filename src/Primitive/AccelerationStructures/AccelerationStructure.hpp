#pragma once

namespace VI
{
struct Ray;
class Scene;
struct Intersection;

class AccelerationStructure
{
public:
  virtual ~AccelerationStructure() = default;
  virtual bool Trace(const Ray& ray, const Scene& scene, Intersection& intersection) const = 0;
  virtual void Build(const Scene& scene) = 0;
};
} // namespace VI
