#include "Primitive/AccelerationStructures/GridAccelerationStructure.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>

#include <glm/common.hpp>

#include "Math/Math.hpp"
#include "Math/Vector.hpp"
#include "Primitive/BoundingBox.hpp"
#include "Primitive/Geometry/Geometry.hpp"
#include "Ray/Intersection.hpp"
#include "Scene/Scene.hpp"

namespace VI {
namespace {

constexpr int GridResolution = 50;
constexpr float MinCellSize = 1e-4f;

float ComputeCellSize(float min, float max, int cell_count) {
  return std::max((max - min) / cell_count, MinCellSize);
}

} // namespace

GridAccelerationStructure
GridAccelerationStructure::Create(const Scene &scene) {
  GridAccelerationStructure accel{};
  accel.Build(scene);

  return accel;
}

bool GridAccelerationStructure::Trace(const Ray &ray, const Scene &scene,
                                      Intersection &intersection) const {
  if (m_Cells.empty() || m_Nx <= 0 || m_Ny <= 0 || m_Nz <= 0)
    return false;

  float tmin, tmax;
  if (!m_BoundingBox.Intersect(ray, tmin, tmax))
    return false;

  Vector entry_point = ray.Origin + ray.Direction * tmin;
  int ix = glm::clamp(int((entry_point.x - m_BoundingBox.Min.x) / m_CellSizeX),
                      0, m_Nx - 1);
  int iy = glm::clamp(int((entry_point.y - m_BoundingBox.Min.y) / m_CellSizeY),
                      0, m_Ny - 1);
  int iz = glm::clamp(int((entry_point.z - m_BoundingBox.Min.z) / m_CellSizeZ),
                      0, m_Nz - 1);

  int step_x = (ray.Direction.x >= 0) ? 1 : -1;
  int step_y = (ray.Direction.y >= 0) ? 1 : -1;
  int step_z = (ray.Direction.z >= 0) ? 1 : -1;

  float next_boundary_x =
      m_BoundingBox.Min.x + (ix + (step_x > 0 ? 1 : 0)) * m_CellSizeX;
  float next_boundary_y =
      m_BoundingBox.Min.y + (iy + (step_y > 0 ? 1 : 0)) * m_CellSizeY;
  float next_boundary_z =
      m_BoundingBox.Min.z + (iz + (step_z > 0 ? 1 : 0)) * m_CellSizeZ;

  float t_max_x = (ray.Direction.x != 0)
                      ? (next_boundary_x - ray.Origin.x) / ray.Direction.x
                      : std::numeric_limits<float>::infinity();
  float t_max_y = (ray.Direction.y != 0)
                      ? (next_boundary_y - ray.Origin.y) / ray.Direction.y
                      : std::numeric_limits<float>::infinity();
  float t_max_z = (ray.Direction.z != 0)
                      ? (next_boundary_z - ray.Origin.z) / ray.Direction.z
                      : std::numeric_limits<float>::infinity();

  float t_delta_x = (ray.Direction.x != 0)
                        ? m_CellSizeX / std::abs(ray.Direction.x)
                        : std::numeric_limits<float>::infinity();
  float t_delta_y = (ray.Direction.y != 0)
                        ? m_CellSizeY / std::abs(ray.Direction.y)
                        : std::numeric_limits<float>::infinity();
  float t_delta_z = (ray.Direction.z != 0)
                        ? m_CellSizeZ / std::abs(ray.Direction.z)
                        : std::numeric_limits<float>::infinity();

  bool hit = false;
  float closestT = tmax + EPSILON;
  Intersection closest_intersection{};

  while (ix >= 0 && ix < m_Nx && iy >= 0 && iy < m_Ny && iz >= 0 && iz < m_Nz) {
    const int cell_index = ix + m_Nx * iy + m_Nx * m_Ny * iz;
    for (int obj_idx : m_Cells[cell_index].ObjectIndices) {
      Intersection temp{};
      if (Intersect(scene.GetPrimitive(obj_idx).Geometry, ray, temp) &&
          temp.Distance < closestT) {
        temp.ObjectIndex = obj_idx;
        closestT = temp.Distance;
        closest_intersection = temp;
        hit = true;
      }
    }

    if (t_max_x < t_max_y) {
      if (t_max_x < t_max_z) {
        ix += step_x;
        t_max_x += t_delta_x;
      } else {
        iz += step_z;
        t_max_z += t_delta_z;
      }
    } else {
      if (t_max_y < t_max_z) {
        iy += step_y;
        t_max_y += t_delta_y;
      } else {
        iz += step_z;
        t_max_z += t_delta_z;
      }
    }

    if (std::min({t_max_x, t_max_y, t_max_z}) > tmax + EPSILON)
      break;
  }

  if (hit) {
    intersection = closest_intersection;
    // std::println("Hit at {}", intersection.ObjectIndex);
    return true;
  }
  return false;
}

void GridAccelerationStructure::Build(const Scene &scene) {
  m_Cells.clear();
  m_Nx = m_Ny = m_Nz = 0;
  m_CellSizeX = m_CellSizeY = m_CellSizeZ = 0.0f;

  if (scene.GetPrimitiveCount() == 0) {
    m_BoundingBox = BoundingBox{};
    return;
  }

  m_BoundingBox = scene.ComputeBoundingBox();

  m_Nx = m_Ny = m_Nz = GridResolution;

  m_CellSizeX = ComputeCellSize(m_BoundingBox.Min.x, m_BoundingBox.Max.x, m_Nx);
  m_CellSizeY = ComputeCellSize(m_BoundingBox.Min.y, m_BoundingBox.Max.y, m_Ny);
  m_CellSizeZ = ComputeCellSize(m_BoundingBox.Min.z, m_BoundingBox.Max.z, m_Nz);

  m_Cells.resize(m_Nx * m_Ny * m_Nz);

  for (int ix = 0; ix < m_Nx; ++ix) {
    for (int iy = 0; iy < m_Ny; ++iy) {
      for (int iz = 0; iz < m_Nz; ++iz) {
        const int cell_index = ix + m_Nx * iy + m_Nx * m_Ny * iz;
        const Vector min =
            m_BoundingBox.Min +
            Vector{ix * m_CellSizeX, iy * m_CellSizeY, iz * m_CellSizeZ};
        const Vector max = min + Vector{m_CellSizeX, m_CellSizeY, m_CellSizeZ};
        m_Cells[cell_index].BoundingBox = BoundingBox{min, max};
      }
    }
  }

  for (size_t i = 0; i < scene.GetPrimitiveCount(); ++i) {
    const auto &obj_bounds = GetBoundingBox(scene.GetPrimitive(i).Geometry);

    const float epsilon = EPSILON;
    Vector min = obj_bounds.Min - Vector{epsilon, epsilon, epsilon};
    Vector max = obj_bounds.Max + Vector{epsilon, epsilon, epsilon};

    const int ix_min =
        std::max(0, std::min(m_Nx - 1,
                             int((min.x - m_BoundingBox.Min.x) / m_CellSizeX)));
    const int iy_min =
        std::max(0, std::min(m_Ny - 1,
                             int((min.y - m_BoundingBox.Min.y) / m_CellSizeY)));
    const int iz_min =
        std::max(0, std::min(m_Nz - 1,
                             int((min.z - m_BoundingBox.Min.z) / m_CellSizeZ)));
    const int ix_max = std::max(
        ix_min,
        std::min(m_Nx - 1, int((max.x - m_BoundingBox.Min.x) / m_CellSizeX)));
    const int iy_max = std::max(
        iy_min,
        std::min(m_Ny - 1, int((max.y - m_BoundingBox.Min.y) / m_CellSizeY)));
    const int iz_max = std::max(
        iz_min,
        std::min(m_Nz - 1, int((max.z - m_BoundingBox.Min.z) / m_CellSizeZ)));

    for (int ix = ix_min; ix <= ix_max; ++ix) {
      for (int iy = iy_min; iy <= iy_max; ++iy) {
        for (int iz = iz_min; iz <= iz_max; ++iz) {
          int cell_index = ix + m_Nx * iy + m_Nx * m_Ny * iz;
          m_Cells[cell_index].ObjectIndices.push_back(i);
        }
      }
    }
  }
}

} // namespace VI
