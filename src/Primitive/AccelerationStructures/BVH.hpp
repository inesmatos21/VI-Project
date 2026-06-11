#pragma once

#include <algorithm>
#include <limits>
#include <span>
#include <vector>

#include "Primitive/BoundingBox.hpp"
#include "Ray/Ray.hpp"

namespace VI
{

// Flattened binary BVH built with a binned Surface Area Heuristic (SAH).
// Generic over any primitive set: Build takes the primitives' bounding boxes
// and Traverse calls back with the indices of potentially hit primitives,
// visiting nodes nearest to the ray origin first.
class BVH
{
public:
  void Build(std::span<const BoundingBox> primitive_bounds);

  // intersect_primitive(index) must intersect that primitive and return the
  // closest hit distance found so far (infinity while there is no hit); it is
  // used to prune nodes that lie beyond the current closest hit.
  template <typename F> void Traverse(const Ray& ray, F&& intersect_primitive) const
  {
    if (m_Nodes.empty())
    {
      return;
    }

    float closest_t = std::numeric_limits<float>::infinity();
    int stack[TraversalStackSize];
    int stack_size = 0;
    int node_index = 0;

    while (true)
    {
      const Node& node = m_Nodes[node_index];
      float tmin, tmax;
      if (node.Bounds.Intersect(ray, tmin, tmax) && tmin <= closest_t)
      {
        if (node.PrimitiveCount > 0)
        {
          for (int i = 0; i < node.PrimitiveCount; ++i)
          {
            closest_t = std::min(closest_t, intersect_primitive(m_PrimitiveIndices[node.FirstPrimitive + i]));
          }
        }
        else if (stack_size < TraversalStackSize)
        {
          // Visit the child on the ray's side of the split plane first.
          const bool negative_direction = ray.Direction[node.SplitAxis] < 0.f;
          const int near_child = negative_direction ? node.SecondChild : node_index + 1;
          const int far_child = negative_direction ? node_index + 1 : node.SecondChild;
          stack[stack_size++] = far_child;
          node_index = near_child;
          continue;
        }
      }

      if (stack_size == 0)
      {
        break;
      }
      node_index = stack[--stack_size];
    }
  }

private:
  static constexpr int TraversalStackSize = 256;

  struct Node
  {
    BoundingBox Bounds{};
    int FirstPrimitive{0}; // leaf only: offset into m_PrimitiveIndices
    int SecondChild{0};    // interior only: index of the right child (the left child is the next node)
    int PrimitiveCount{0}; // 0 for interior nodes
    int SplitAxis{0};      // interior only
  };

  int BuildRecursive(std::span<const BoundingBox> primitive_bounds, std::span<const Point> centroids, int start, int end);

  std::vector<Node> m_Nodes{};
  std::vector<int> m_PrimitiveIndices{};
};

} // namespace VI
