#include "Image/FileImages.hpp"

#include "Image/Image.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>

namespace VI
{

Image ImagePPM::Load(const std::string& filename [[maybe_unused]])
{
  throw std::runtime_error("Not implemented");
}

bool ImagePPM::Save(const Image& image, const std::string& filename)
{
  std::ofstream file;
  file.open(filename.data(), std::ios::out); // Text mode is fine for P3

  if (file.fail())
  {
    std::cout << "Failed to open file" << std::endl;
    return false;
  }

  int width = image.GetWidth();
  int height = image.GetHeight();

  file << "P3\n" << width << " " << height << "\n255\n";
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      const auto& data = image.Get(x, y);

      // Tone mapping (Reinhard)
      float r = data.r / (1.0f + data.r);
      float g = data.g / (1.0f + data.g);
      float b = data.b / (1.0f + data.b);

      // Gamma correction
      r = pow(r, 1.0f / 2.2f);
      g = pow(g, 1.0f / 2.2f);
      b = pow(b, 1.0f / 2.2f);

      // Clamp and convert to 8-bit
      int r8 = static_cast<int>(std::clamp(r, 0.0f, 1.0f) * 255.0f);
      int g8 = static_cast<int>(std::clamp(g, 0.0f, 1.0f) * 255.0f);
      int b8 = static_cast<int>(std::clamp(b, 0.0f, 1.0f) * 255.0f);

      file << r8 << " " << g8 << " " << b8 << "\n";
    }
  }

  return true;
} // namespace VI

} // namespace VI
