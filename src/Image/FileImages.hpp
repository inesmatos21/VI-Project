#pragma once

#include "Image/Image.hpp"

#include <string>

namespace VI
{
class ImagePPM
{
public:
  static Image Load(const std::string& filename);
  static bool Save(const Image& image, const std::string& filename);
};
} // namespace VI
