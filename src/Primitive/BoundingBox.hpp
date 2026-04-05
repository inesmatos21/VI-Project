#pragma once

#include "Math/Vector.hpp"
#include "Ray/Ray.hpp"

namespace VI
{
constexpr float MachineEpsilon = std::numeric_limits<float>::epsilon() * 0.5;

constexpr float gamma(int n)
{
  return (n * MachineEpsilon) / (1 - n * MachineEpsilon);
}

struct BoundingBox
{
  Point Min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  Point Max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

  constexpr void Update(const Point& p)
  {
    if (p.x < Min.x)
    {
      Min.x = p.x;
    }
    else if (p.x > Max.x)
    {
      Max.x = p.x;
    }
    if (p.y < Min.y)
    {
      Min.y = p.y;
    }
    else if (p.y > Max.y)
    {
      Max.y = p.y;
    }
    if (p.z < Min.z)
    {
      Min.z = p.z;
    }
    else if (p.z > Max.z)
    {
      Max.z = p.z;
    }
  }

  /*
   * I suggest you implement:
   *  bool intersect (Ray r) { }
   *
   * based on PBRT's 3rd ed. book , sec 3.1.2, pags 125..128 + 214,217,221
   *
   * or https://doi.org/10.1007/978-1-4842-7185-8_2
   *
   */
  constexpr bool Intersect(const Ray& ray [[maybe_unused]], float& tmin [[maybe_unused]], float& tmax [[maybe_unused]]) const
  {
    return true;
  }

  constexpr void Update(const BoundingBox& other)
  {
    Min.x = glm::min(Min.x, other.Min.x);
    Min.y = glm::min(Min.y, other.Min.y);
    Min.z = glm::min(Min.z, other.Min.z);
    Max.x = glm::max(Max.x, other.Max.x);
    Max.y = glm::max(Max.y, other.Max.y);
    Max.z = glm::max(Max.z, other.Max.z);
  }
};
} // namespace VI
