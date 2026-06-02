#pragma once

#include "Math/Vector.hpp"
#include "Primitive/BoundingBox.hpp"

namespace VI
{
struct Ray;
struct Intersection;

class Sphere final
{
public:
  // Stationary sphere
  Sphere(Point center, float radius) 
    : m_Center1(center), m_Center2(center), m_Radius(radius), 
    m_BoundingBox(center - radius, center + radius) {}

  // Moving Sphere : interpolates from center1 (t=0) to center2 (t=1)
  Sphere(Point center1, Point center2, float radius)
    : m_Center1(center1), m_Center2(center2), m_Radius(radius),
    m_BoundingBox(
      glm::min(center1 - radius, center2 - radius),
      glm::max(center1 + radius, center2 + radius)) {}

  bool Intersect(const Ray& r, Intersection& i) const;

  inline const BoundingBox& GetBoundingBox() const
  {
    return m_BoundingBox;
  }

private:
  // Returns the sphere center at the ray's time (linear interpolation)
  Point CenterAt(float time) const {
    return m_Center1 + time * (m_Center2 - m_Center1);
  }

  Point m_Center1;
  Point m_Center2;
  float m_Radius;
  BoundingBox m_BoundingBox;
};
} // namespace VI
