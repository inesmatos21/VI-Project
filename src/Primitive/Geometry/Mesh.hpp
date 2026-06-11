#pragma once

#include "Primitive/AccelerationStructures/BVH.hpp"
#include "Primitive/BoundingBox.hpp"
#include "Primitive/Geometry/Triangle.hpp"

namespace VI
{

struct Ray;
struct Intersection;

class Mesh final
{
public:
  Mesh(std::string_view name, std::vector<Triangle> triangles) : m_Name{name}, m_Triangles{std::move(triangles)}
  {
    // Compute bounding box from triangles
    if (!m_Triangles.empty())
    {
      m_BoundingBox = BoundingBox{};
      for (const auto& tri : m_Triangles)
      {
        const auto& [v1, v2, v3] = tri.GetVertices();
        m_BoundingBox.Update(v1);
        m_BoundingBox.Update(v2);
        m_BoundingBox.Update(v3);
      }
    }
    BuildBVH();
  }

  bool Intersect(const Ray& r, Intersection& i) const;
  const BoundingBox& GetBoundingBox() const;
  size_t GetTriangleCount() const noexcept;
  const Triangle& GetTriangle(size_t i) const;
  void AddTriangle(const Triangle& triangle);
  float GetArea() const noexcept;

private:
  void BuildBVH();

  std::string m_Name;
  std::vector<Triangle> m_Triangles{};
  BoundingBox m_BoundingBox{};
  BVH m_BVH{};
};
} // namespace VI
