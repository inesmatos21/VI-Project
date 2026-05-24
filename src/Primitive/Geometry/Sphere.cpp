#include "Primitive/Geometry/Sphere.hpp"

#include <glm/geometric.hpp>

#include "Math/Math.hpp"
#include "Ray/Intersection.hpp"
#include "Ray/Ray.hpp"

namespace VI
{

bool Sphere::Intersect(const Ray& ray, Intersection& intersection) const
{
  // NOTE: bounding box test is intentionally skipped here.
  // The GridAccelerationStructure already culls by bbox; testing again inside
  // Intersect causes rectangular artefacts with moving spheres because the
  // bbox covers the full t=0..1 trajectory.

  // Motion blur: use the sphere center at the ray's shutter time.
  Point current_center = CenterAt(ray.Time);

  Vector oc = current_center - ray.Origin;

  float h = glm::dot(ray.Direction, oc);
  float c = glm::dot(oc, oc) - (m_Radius * m_Radius);
  float discriminant = h * h - c;
  if (discriminant < EPSILON)
    return false;

  float sqrt_d = std::sqrt(discriminant);
  float t = h - sqrt_d;

  // If the near root is behind/too close, try the far root.
  // This handles rays originating inside the sphere (e.g. refracted ray
  // travelling through a dielectric and hitting the exit surface).
  if (t <= EPSILON)
  {
    t = h + sqrt_d;
    if (t <= EPSILON)
      return false;
  }

  Point hit_point = ray.Origin + t * ray.Direction;

  // Outward normal always points away from the sphere centre.
  Vector outward_normal = glm::normalize(hit_point - current_center);

  // Determine whether the ray hits the front (outside) or back (inside).
  // front_face = true  → ray is outside the sphere (dot < 0 means opposing dirs)
  // front_face = false → ray is inside  the sphere
  bool front_face = glm::dot(ray.Direction, outward_normal) < 0.f;

  // The shading normal always opposes the incoming ray so that lighting
  // calculations work correctly from both sides.
  Vector shading_normal = front_face ? outward_normal : -outward_normal;

  intersection = {
      .Position  = hit_point,
      .Normal    = shading_normal,
      .Distance  = t,
      .FrontFace = front_face,
  };

  return true;
}

} // namespace VI