#pragma once

#include "Math/Math.hpp"
#include "Math/Vector.hpp"

namespace VI
{
struct Ray
{
  Point Origin;
  Vector Direction;
  float Time{0.f};

  static Ray WithOffset(const Point& origin, const Vector& direction, const Vector& normal, float time =0.f, float epsilon = EPSILON)
  {
    return {.Origin = OffsetPoint(origin, normal, epsilon), .Direction = direction, .Time = time};
  }
};
} // namespace VI
