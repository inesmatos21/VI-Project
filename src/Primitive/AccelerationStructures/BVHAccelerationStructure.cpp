#include "Primitive/AccelerationStructures/BVHAccelerationStructure.hpp"

#include <limits>
#include <vector>

#include "Primitive/BoundingBox.hpp"
#include "Primitive/Geometry/Geometry.hpp"
#include "Ray/Intersection.hpp"
#include "Scene/Scene.hpp"

namespace VI
{

BVHAccelerationStructure BVHAccelerationStructure::Create(const Scene& scene)
{
  BVHAccelerationStructure accel{};
  accel.Build(scene);
  return accel;
}

void BVHAccelerationStructure::Build(const Scene& scene)
{
  std::vector<BoundingBox> bounds;
  bounds.reserve(scene.GetPrimitiveCount());
  for (size_t i = 0; i < scene.GetPrimitiveCount(); ++i)
  {
    bounds.push_back(GetBoundingBox(scene.GetPrimitive(i).Geometry));
  }
  m_BVH.Build(bounds);
}

bool BVHAccelerationStructure::Trace(const Ray& ray, const Scene& scene, Intersection& intersection) const
{
  bool hit = false;
  Intersection closest{};

  m_BVH.Traverse(ray, [&](int object_index) {
    Intersection temp{};
    if (Intersect(scene.GetPrimitive(object_index).Geometry, ray, temp) && (!hit || temp.Distance < closest.Distance))
    {
      temp.ObjectIndex = object_index;
      closest = temp;
      hit = true;
    }
    return hit ? closest.Distance : std::numeric_limits<float>::infinity();
  });

  if (hit)
  {
    intersection = closest;
  }
  return hit;
}

} // namespace VI
