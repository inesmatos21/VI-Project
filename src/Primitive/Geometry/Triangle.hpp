#pragma once

#include "Math/Vector.hpp"
#include "Primitive/BoundingBox.hpp"

#include <tuple>

namespace VI
{
struct Ray;
struct Intersection;

class Triangle final
{
public:
  Triangle(const Point& v1, const Point& v2, const Point& v3, const Vector& normal, bool back_face_culling = false);
  Triangle(const Point& v1, const Point& v2, const Point& v3, const Vector& normal, const Vec2& uv1, const Vec2& uv2, const Vec2& uv3, bool back_face_culling = false);

  bool Intersect(const Ray& r, Intersection& i) const;
  const BoundingBox& GetBoundingBox() const;

  std::tuple<Point, Point, Point> GetVertices() const;
  std::tuple<Vec2, Vec2, Vec2> GetTexCoords() const;
  Vector GetNormal() const noexcept;
  float GetArea() const noexcept;

private:
  Point m_V1, m_V2, m_V3;
  Vector m_Normal;
  Vec2 m_UV1{0.f}, m_UV2{0.f}, m_UV3{0.f};
  BoundingBox m_BoundingBox;
  bool m_BackFaceCulling;
  float m_Area;
};
} // namespace VI
