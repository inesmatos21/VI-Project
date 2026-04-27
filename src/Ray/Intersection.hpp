#pragma once

#include "Math/Vector.hpp"

namespace VI
{

struct Intersection
{
  Vector Position;
  Vector Normal;
  Vec2 TexCoord{0.f};
  float Distance{-1};
  int ObjectIndex{-1};
  int PrimitiveIndex{-1};
};

} // namespace VI
