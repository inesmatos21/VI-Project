#include "Image/Denoiser.hpp"

#include <OpenImageDenoise/oidn.hpp>

#include <cstddef>
#include <cstring>
#include <format>
#include <stdexcept>

#include "Image/Image.hpp"

namespace VI
{
Denoiser::Denoiser(int width, int height, bool has_albedo, bool has_normals)
    : m_Width{width},
      m_Height{height},
      m_HasAlbedo{has_albedo},
      m_HasNormals{has_normals}
{
  m_Width = width;
  m_Height = height;
  m_HasAlbedo = has_albedo;
  m_HasNormals = has_normals;
  // create device
  m_Device = oidn::newDevice(oidn::DeviceType::CPU);
  m_Device.commit();

  // create buffers
  // Create buffers for input/output images accessible by both host (CPU) and device (CPU/GPU)
  m_ColorBuffer = m_Device.newBuffer(m_Width * m_Height * 3 * sizeof(float));
  if (m_HasNormals)
  {
    m_NormalBuffer = m_Device.newBuffer(m_Width * m_Height * 3 * sizeof(float));
  }
  if (m_HasAlbedo)
  {
    m_AlbedoBuffer = m_Device.newBuffer(m_Width * m_Height * 3 * sizeof(float));
  }

  // Create a filter for denoising a beauty (color) image using optional auxiliary images too
  // This can be an expensive operation, so try no to create a new filter for every image!
  m_Filter = m_Device.newFilter("RT");                                                 // generic ray tracing filter
  m_Filter.setImage("color", m_ColorBuffer, oidn::Format::Float3, m_Width, m_Height);  // beauty
  m_Filter.setImage("output", m_ColorBuffer, oidn::Format::Float3, m_Width, m_Height); // denoised beauty
  if (m_HasAlbedo)
  {
    m_Filter.setImage("albedo", m_AlbedoBuffer, oidn::Format::Float3, m_Width, m_Height); // auxiliary
  }
  if (m_HasNormals)
  {
    m_Filter.setImage("normal", m_NormalBuffer, oidn::Format::Float3, m_Width, m_Height); // auxiliary
  }
  m_Filter.set("hdr", true); // beauty image is HDR
  m_Filter.commit();
}

Image Denoiser::Execute(const Image& image, float* albedo, float* normals)
{
  if (m_HasAlbedo && albedo == nullptr)
  {
    throw std::invalid_argument{"Denoiser was created with albedo enabled, but no albedo buffer was provided"};
  }

  if (m_HasNormals && normals == nullptr)
  {
    throw std::invalid_argument{"Denoiser was created with normals enabled, but no normal buffer was provided"};
  }

  CopyInputImage(image);

  if (m_HasAlbedo)
    CopyToBuffer(m_AlbedoBuffer, albedo);

  if (m_HasNormals)
    CopyToBuffer(m_NormalBuffer, normals);

  m_Filter.execute();
  CheckOidnError("OIDN filter execution failed");

  const auto* denoised_ptr = static_cast<const float*>(m_ColorBuffer.getData());

  Image denoised_image{m_Width, m_Height, denoised_ptr};
  return denoised_image;
}

[[nodiscard]] std::size_t Denoiser::PixelCount() const
{
  return static_cast<std::size_t>(m_Width) * static_cast<std::size_t>(m_Height);
}

[[nodiscard]] std::size_t Denoiser::BufferSizeBytes() const
{
  return PixelCount() * 3 * sizeof(float);
}

void Denoiser::CopyInputImage(const Image& image)
{
  const auto* image_ptr = image.GetFloatPtr();
  CopyToBuffer(m_ColorBuffer, image_ptr);
}

void Denoiser::CopyToBuffer(const oidn::BufferRef& buffer, const float* source)
{
  auto* destination = static_cast<float*>(buffer.getData());
  std::memcpy(destination, source, BufferSizeBytes());
}

void Denoiser::CheckOidnError(const char* context)
{
  char const* error_message = nullptr;

  if (m_Device.getError(error_message) != oidn::Error::None)
  {
    throw std::runtime_error{std::format("{}: {}", context, error_message != nullptr ? error_message : "unknown error")};
  }
}

} // namespace VI
