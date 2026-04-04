#include "Primitive/Geometry/Mesh.hpp"

#include "Ray/Intersection.hpp"
#include "Ray/Ray.hpp"

namespace VI {

bool Mesh::Intersect(const Ray &r, Intersection &intersection) const {
  float tmin, tmax;
  if (!m_BoundingBox.Intersect(r, tmin, tmax)) {
    return false;
  }

  intersection.Distance = -1;

  for (size_t i = 0; i < m_Triangles.size(); i++) {
    const auto &triangle = m_Triangles[i];

    Intersection temp_intersection{};

    if (triangle.Intersect(r, temp_intersection)) {
      if (intersection.Distance == -1 ||
          intersection.Distance > temp_intersection.Distance) {
        intersection = temp_intersection;
        intersection.PrimitiveIndex = i;
      }
    }
  }

  return intersection.Distance != -1;
}

const BoundingBox &Mesh::GetBoundingBox() const { return m_BoundingBox; }

size_t Mesh::GetTriangleCount() const noexcept { return m_Triangles.size(); }

const Triangle &Mesh::GetTriangle(size_t i) const { return m_Triangles[i]; }

void Mesh::AddTriangle(const Triangle &triangle) {
  m_Triangles.push_back(triangle);
}

float Mesh::GetArea() const noexcept {
  float total_area{0.f};

  for (const auto &triangle : m_Triangles) {
    total_area += triangle.GetArea();
  }

  return total_area;
}

} // namespace VI
