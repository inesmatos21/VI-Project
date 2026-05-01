
#pragma once

#include "Image/Image.hpp"
#include "OpenImageDenoise/oidn.hpp"
#include "Math/Vector.hpp"

namespace VI
{
class Denoiser
{
public:
    Denoiser (int const _w, int const _h, bool const _hasAlbedo=false, bool const _hasNormals=false) {
        
        w = _w; h = _h;
        hasAlbedo = _hasAlbedo; hasNormals = _hasNormals;
        // create device
        device = oidn::newDevice(oidn::DeviceType::CPU);
        device.commit();

        // create buffers
        // Create buffers for input/output images accessible by both host (CPU) and device (CPU/GPU)
        colorBuf = device.newBuffer(w * h * 3 * sizeof(float));
        if (hasNormals) {
            normalBuf = device.newBuffer(w * h * 3 * sizeof(float));
        }
        if (hasAlbedo) {
            albedoBuf = device.newBuffer(w * h * 3 * sizeof(float));
        }

        // Create a filter for denoising a beauty (color) image using optional auxiliary images too
        // This can be an expensive operation, so try no to create a new filter for every image!
        filter = device.newFilter("RT"); // generic ray tracing filter
        filter.setImage("color", colorBuf, oidn::Format::Float3, w, h); // beauty
        filter.setImage("output", colorBuf, oidn::Format::Float3, w, h); // denoised beauty
        if (hasAlbedo) {
            filter.setImage("albedo", albedoBuf, oidn::Format::Float3, w, h); // auxiliary
        }
        if (hasNormals) {
            filter.setImage("normal", normalBuf, oidn::Format::Float3, w, h); // auxiliary
        }
        filter.set("hdr", true); // beauty image is HDR
        filter.commit();
    }
    
    Image execute (Image const image, float *albedo=NULL, float *normals=NULL) {
        
        // Fill the input image buffers
        float* colorPtr = (float*)colorBuf.getData();
        float* imagePtr = image.GetFloatPtr();
        std::memcpy(colorPtr, imagePtr, w * h * 3 * sizeof(float));

        if (hasAlbedo && albedo!=NULL) {
            float* albedoPtr = (float*)albedoBuf.getData();
            std::memcpy(albedoPtr, albedo, w * h * 3 * sizeof(float));
        }
        if (hasNormals && normals!=NULL) {
            float* normalPtr = (float*)normalBuf.getData();
            std::memcpy(normalPtr, normals, w * h * 3 * sizeof(float));
        }
        // Filter the beauty image
        filter.execute();

        // Check for errors
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cout << "Error: " << errorMessage << std::endl;
        }
        
        // Get the output buffer
        //float* DenoisedPtr = (float*)colorBuf.getData();
        Image denoised_image{w, h};
        denoised_image.SetFromBuffer (colorPtr);
        
        return denoised_image;

    }
private:
    oidn::DeviceRef device;
    oidn::BufferRef colorBuf, normalBuf, albedoBuf;
    oidn::FilterRef filter;
    int w,h;
    bool hasAlbedo, hasNormals;
};
} // namespace VI


