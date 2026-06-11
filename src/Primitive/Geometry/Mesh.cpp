#include "Primitive/Geometry/Mesh.hpp"

#include <limits>

#include "Ray/Intersection.hpp"
#include "Ray/Ray.hpp"

namespace VI
{

bool Mesh::Intersect(const Ray& r, Intersection& intersection) const
{
  intersection.Distance = -1;

  bool hit = false;
  Intersection closest{};

  m_BVH.Traverse(r, [&](int triangle_index) {
    Intersection temp{};
    if (m_Triangles[triangle_index].Intersect(r, temp) && (!hit || temp.Distance < closest.Distance))
    {
      temp.PrimitiveIndex = triangle_index;
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

void Mesh::BuildBVH()
{
  std::vector<BoundingBox> bounds;
  bounds.reserve(m_Triangles.size());
  for (const auto& triangle : m_Triangles)
  {
    bounds.push_back(triangle.GetBoundingBox());
  }
  m_BVH.Build(bounds);
}

const BoundingBox& Mesh::GetBoundingBox() const
{
  return m_BoundingBox;
}

size_t Mesh::GetTriangleCount() const noexcept
{
  return m_Triangles.size();
}

const Triangle& Mesh::GetTriangle(size_t i) const
{
  return m_Triangles[i];
}

void Mesh::AddTriangle(const Triangle& triangle)
{
  m_Triangles.push_back(triangle);
  m_BoundingBox.Update(triangle.GetBoundingBox());
  BuildBVH();
}

float Mesh::GetArea() const noexcept
{
  float total_area{0.f};

  for (const auto& triangle : m_Triangles)
  {
    total_area += triangle.GetArea();
  }

  return total_area;
}

} // namespace VI
