#pragma once

#include <OpenImageDenoise/oidn.hpp>

#include <cstddef>

namespace VI
{
class Image;

class Denoiser
{
public:
  Denoiser(int width, int height, bool has_albedo = false, bool has_normals = false);

  Image Execute(const Image& image, float* albedo = nullptr, float* normals = nullptr);

private:
  [[nodiscard]] std::size_t PixelCount() const;

  [[nodiscard]] std::size_t BufferSizeBytes() const;

  void CopyInputImage(const Image& image);

  void CopyToBuffer(const oidn::BufferRef& buffer, const float* source);

  void CheckOidnError(const char* context);

  oidn::DeviceRef m_Device;
  oidn::BufferRef m_ColorBuffer, m_NormalBuffer, m_AlbedoBuffer;
  oidn::FilterRef m_Filter;
  int m_Width, m_Height;
  bool m_HasAlbedo, m_HasNormals;
};

} // namespace VI
