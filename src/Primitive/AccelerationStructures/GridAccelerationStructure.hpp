#pragma once

#include <vector>

#include "Primitive/AccelerationStructures/AccelerationStructure.hpp"
#include "Primitive/BoundingBox.hpp"

namespace VI
{
struct GridCell
{
  std::vector<int> ObjectIndices;
  BoundingBox BoundingBox;
};

class GridAccelerationStructure final : public AccelerationStructure
{
public:
  static GridAccelerationStructure Create(const Scene& scene);
  bool Trace(const Ray& ray, const Scene& scene, Intersection& intersection) const override;
  void Build(const Scene& scene) override;

private:
  std::vector<GridCell> m_Cells [[maybe_unused]]{};
  int m_Nx [[maybe_unused]]{0}, m_Ny [[maybe_unused]]{0}, m_Nz [[maybe_unused]]{0};
  BoundingBox m_BoundingBox [[maybe_unused]]{};
  float m_CellSizeX [[maybe_unused]]{0.0f}, m_CellSizeY [[maybe_unused]]{0.0f}, m_CellSizeZ [[maybe_unused]]{0.0f};
};

} // namespace VI
