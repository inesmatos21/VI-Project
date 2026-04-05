#include "Math/Random.hpp"
#include "Math/Vector.hpp"

#include <cstdint>
#include <random>

namespace VI
{
namespace
{
uint64_t SplitMix64(uint64_t& state)
{
  state += 0x9e3779b97f4a7c15ULL;
  uint64_t z = state;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}
} // namespace

void Random::Seed(uint64_t seed)
{
  uint64_t state = seed;
  std::seed_seq seq{
      static_cast<uint32_t>(SplitMix64(state)),
      static_cast<uint32_t>(SplitMix64(state) >> 32),
      static_cast<uint32_t>(SplitMix64(state)),
      static_cast<uint32_t>(SplitMix64(state) >> 32),
  };
  s_Rng.seed(seq);
}

float Random::RandomFloat(float min, float max)
{
  std::uniform_real_distribution<float> dist{min, max};
  return dist(s_Rng);
}

Vector Random::RandomVec3(float min, float max)
{
  return {
      RandomFloat(min, max),
      RandomFloat(min, max),
      RandomFloat(min, max),
  };
}

Point Random::RandomInUnitDisk()
{
  while (true)
  {
    Point p = RandomVec3();

    if ((p.x * p.x + p.y * p.y) < 1.f)
    {
      return p;
    }
  }
}

thread_local std::mt19937 Random::s_Rng;

} // namespace VI
