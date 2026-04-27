#include "Primitive/Geometry/Triangle.hpp"

#include "Math/Math.hpp"
#include "Ray/Intersection.hpp"
#include "Ray/Ray.hpp"

namespace VI
{

Triangle::Triangle(const Point& v1, const Point& v2, const Point& v3, const Vector& normal, bool back_face_culling) : Triangle{v1, v2, v3, normal, Vec2{0.f}, Vec2{0.f}, Vec2{0.f}, back_face_culling}
{
}

Triangle::Triangle(const Point& v1, const Point& v2, const Point& v3, const Vector& normal, const Vec2& uv1, const Vec2& uv2, const Vec2& uv3, bool back_face_culling) : m_V1{v1}, m_V2{v2}, m_V3{v3}, m_Normal{normal}, m_UV1{uv1}, m_UV2{uv2}, m_UV3{uv3}, m_BackFaceCulling{back_face_culling}, m_Area{0.5f * glm::length(glm::cross(m_V2 - m_V1, m_V3 - m_V1))}
{
  m_BoundingBox = {.Min = v1, .Max = v1};
  m_BoundingBox.Update(v2);
  m_BoundingBox.Update(v3);
}

bool Triangle::Intersect(const Ray& r, Intersection& intersection) const
{
  float tmin, tmax;
  if (!m_BoundingBox.Intersect(r, tmin, tmax))
  {
    return false;
  }
  const float par = glm::dot(m_Normal, r.Direction);
  if ((m_BackFaceCulling && par > -EPSILON) || (!m_BackFaceCulling && glm::abs(par) < EPSILON))
  {
    return false;
  }

  Vector edge1 = m_V2 - m_V1;
  Vector edge2 = m_V3 - m_V1;

  Vector h, s, q;
  float a, ff, u, v;

  h = glm::cross(r.Direction, edge2);
  a = glm::dot(edge1, h);
  ff = 1.0 / a;
  s = r.Origin - m_V1;
  u = ff * glm::dot(s, h);

  if (u < 0.0 || u > 1.0)
  {
    return false;
  }

  q = glm::cross(s, edge1);
  v = ff * glm::dot(r.Direction, q);

  if (v < 0.0 || u + v > 1.0)
  {
    return false;
  }

  float t = ff * glm::dot(edge2, q);
  if (t <= EPSILON)
    return false;

  Point p_hit = r.Origin + t * r.Direction;

  intersection = {
      .Position = p_hit,
      .Normal = m_Normal,
      .TexCoord = (1.0f - u - v) * m_UV1 + u * m_UV2 + v * m_UV3,
      .Distance = t,
  };

  return true;
}

const BoundingBox& Triangle::GetBoundingBox() const
{
  return m_BoundingBox;
}

std::tuple<Point, Point, Point> Triangle::GetVertices() const
{
  return std::make_tuple(m_V1, m_V2, m_V3);
}

Vector Triangle::GetNormal() const noexcept
{
  return m_Normal;
}

float Triangle::GetArea() const noexcept
{
  return m_Area;
}

} // namespace VI
