#include "Primitive/AccelerationStructures/BVH.hpp"

#include <numeric>

#include "Math/Vector.hpp"

namespace VI
{
namespace
{

constexpr int NumBins = 12;
constexpr int MaxLeafSize = 4;
// Cost of one bounding-box test relative to one primitive intersection,
// used by the SAH to decide between splitting and creating a leaf.
constexpr float TraversalCost = 0.125f;

float SurfaceArea(const BoundingBox& box)
{
  const Vector extent = box.Max - box.Min;
  if (extent.x < 0.f || extent.y < 0.f || extent.z < 0.f)
  {
    return 0.f;
  }
  return 2.f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
}

struct Bin
{
  BoundingBox Bounds{};
  int Count{0};
};

} // namespace

void BVH::Build(std::span<const BoundingBox> primitive_bounds)
{
  m_Nodes.clear();
  m_PrimitiveIndices.clear();

  const int primitive_count = static_cast<int>(primitive_bounds.size());
  if (primitive_count == 0)
  {
    return;
  }

  m_PrimitiveIndices.resize(primitive_count);
  std::iota(m_PrimitiveIndices.begin(), m_PrimitiveIndices.end(), 0);

  std::vector<Point> centroids(primitive_count);
  for (int i = 0; i < primitive_count; ++i)
  {
    centroids[i] = 0.5f * (primitive_bounds[i].Min + primitive_bounds[i].Max);
  }

  m_Nodes.reserve(2 * static_cast<size_t>(primitive_count));
  BuildRecursive(primitive_bounds, centroids, 0, primitive_count);
}

int BVH::BuildRecursive(std::span<const BoundingBox> primitive_bounds, std::span<const Point> centroids, int start, int end)
{
  const int node_index = static_cast<int>(m_Nodes.size());
  m_Nodes.emplace_back();

  BoundingBox bounds{};
  BoundingBox centroid_bounds{};
  for (int i = start; i < end; ++i)
  {
    bounds.Update(primitive_bounds[m_PrimitiveIndices[i]]);
    centroid_bounds.Update(centroids[m_PrimitiveIndices[i]]);
  }

  const int count = end - start;
  const auto make_leaf = [&] {
    m_Nodes[node_index] = Node{.Bounds = bounds, .FirstPrimitive = start, .PrimitiveCount = count};
    return node_index;
  };

  if (count <= 2)
  {
    return make_leaf();
  }

  const Vector centroid_extent = centroid_bounds.Max - centroid_bounds.Min;
  int axis = 0;
  if (centroid_extent.y > centroid_extent[axis])
  {
    axis = 1;
  }
  if (centroid_extent.z > centroid_extent[axis])
  {
    axis = 2;
  }
  if (centroid_extent[axis] <= 0.f)
  {
    // All centroids coincide: no split can separate the primitives.
    return make_leaf();
  }

  // Binned SAH: bucket the primitives by centroid along the chosen axis and
  // evaluate the split cost after each bin boundary.
  Bin bins[NumBins]{};
  const auto bin_of = [&](int primitive) {
    const float offset = (centroids[primitive][axis] - centroid_bounds.Min[axis]) / centroid_extent[axis];
    return std::clamp(static_cast<int>(offset * NumBins), 0, NumBins - 1);
  };
  for (int i = start; i < end; ++i)
  {
    Bin& bin = bins[bin_of(m_PrimitiveIndices[i])];
    bin.Bounds.Update(primitive_bounds[m_PrimitiveIndices[i]]);
    ++bin.Count;
  }

  // Sweep from the right to accumulate the cost of each possible right side,
  // then from the left to find the cheapest split.
  float right_cost[NumBins - 1];
  {
    BoundingBox right_bounds{};
    int right_count = 0;
    for (int i = NumBins - 1; i >= 1; --i)
    {
      right_bounds.Update(bins[i].Bounds);
      right_count += bins[i].Count;
      right_cost[i - 1] = SurfaceArea(right_bounds) * right_count;
    }
  }

  int best_split = 0;
  float best_cost = std::numeric_limits<float>::infinity();
  {
    BoundingBox left_bounds{};
    int left_count = 0;
    for (int i = 0; i < NumBins - 1; ++i)
    {
      left_bounds.Update(bins[i].Bounds);
      left_count += bins[i].Count;
      const float cost = SurfaceArea(left_bounds) * left_count + right_cost[i];
      if (cost < best_cost)
      {
        best_cost = cost;
        best_split = i;
      }
    }
  }

  if (count <= MaxLeafSize)
  {
    const float leaf_cost = static_cast<float>(count);
    const float split_cost = TraversalCost + best_cost / SurfaceArea(bounds);
    if (!(split_cost < leaf_cost))
    {
      return make_leaf();
    }
  }

  auto mid_iter = std::partition(m_PrimitiveIndices.begin() + start, m_PrimitiveIndices.begin() + end,
                                 [&](int primitive) { return bin_of(primitive) <= best_split; });
  int mid = static_cast<int>(mid_iter - m_PrimitiveIndices.begin());
  if (mid == start || mid == end)
  {
    // Degenerate binning (e.g. all centroids in one bin): median split instead.
    mid = start + count / 2;
    std::nth_element(m_PrimitiveIndices.begin() + start, m_PrimitiveIndices.begin() + mid, m_PrimitiveIndices.begin() + end,
                     [&](int a, int b) { return centroids[a][axis] < centroids[b][axis]; });
  }

  m_Nodes[node_index].Bounds = bounds;
  m_Nodes[node_index].SplitAxis = axis;
  m_Nodes[node_index].PrimitiveCount = 0;
  BuildRecursive(primitive_bounds, centroids, start, mid);
  m_Nodes[node_index].SecondChild = BuildRecursive(primitive_bounds, centroids, mid, end);
  return node_index;
}

} // namespace VI
