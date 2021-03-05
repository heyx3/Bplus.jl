#include "CommonLoaderTypes.h"

#include "../IO.h"

using namespace Bplus;
using namespace Bplus::Assets;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


namespace {
    thread_local std::optional<jpeg_decompress_struct> turboJPEGThreadInstance = std::nullopt;
    auto FindTurboJPEG()
    {
        //Refer to: https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/master/example.txt
        //          https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/master/libjpeg.txt

        auto& threadReference = turboJPEGThreadInstance;
        if (!threadReference.has_value())
        {
            threadReference = jpeg_decompress_struct();
            threadReference->err = jpeg_std_error(threadReference->err);
            jpeg_create_decompress(&threadReference.value());
        }
        return *threadReference;
    }

    thread_local std::optional<png_structp> libPNGThreadInstance_struct = std::nullopt;
    thread_local png_infop libPNGThreadInstance_info = nullptr;
    auto FindLibPNG()
    {
        //Refer to: https://github.com/glennrp/libpng/blob/libpng16/libpng-manual.txt

        auto& threadRef_struct = libPNGThreadInstance_struct;
        auto& threadRef_info = libPNGThreadInstance_info;

        if (!threadRef_struct.has_value())
        {
            threadRef_struct = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                      nullptr, nullptr, nullptr);
            BP_ASSERT(*threadRef_struct != nullptr, "Unable to initialize a libPNG reader");

            threadRef_info = png_create_info_struct(*threadRef_struct);
            BP_ASSERT(threadRef_info != nullptr,
                      "Unable to initialize a libPNG 'info' struct for reading");
        }

        return std::make_tuple(*threadRef_struct, threadRef_info);
    }
}



std::optional<ImageLoaderFormats> Bplus::Assets::GuessImageFormat(const std::string& extension)
{
    BP_ASSERT(extension.size() < 1 || extension[0] != '.',
              "Extension shouldn't start with a dot");
    for (auto [format, extensions] : GetDefaultExtensions())
        for (const char* potentialExt : extensions)
            if (extension == potentialExt)
                return format;
    return std::nullopt;
}

std::optional<ImageLoaderFormats> ImageLoader::GetFormat() const
{
    if (ForcedFileFormat.has_value())
        return ForcedFileFormat.value();
    else
        return GuessImageFormat(fs::path(Path).extension().string());
}
bool ImageLoader::IsPathValid() const
{
    return GetFormat().has_value() &&
           ContentFolderLoader<GL::Textures::Texture2D>::IsPathValid();
}

bool ImageLoader::ProcessAfterRetrieve(std::vector<std::byte>&& diskData)
{
    switch (*GetFormat())
    {
        case ImageLoaderFormats::PNG: {
            auto [pngStruct, pngInfo] = FindLibPNG();

            //Configure default parameters and handling of alpha.
            png_set_gamma(pngStruct, PNG_GAMMA_LINEAR, PNG_GAMMA_LINEAR);
            png_set_alpha_mode(pngStruct, PNG_ALPHA_PNG, PNG_GAMMA_LINEAR);

            //Read the header information.
            png_read_info(pngStruct, pngInfo);
            pixelSize = { png_get_image_width(pngStruct, pngInfo),
                          png_get_image_height(pngStruct, pngInfo) };
            auto colorType = png_get_color_type(pngStruct, pngInfo);
            auto bitDepth = png_get_bit_depth(pngStruct, pngInfo);
            bool hasSeparateAlpha = png_get_valid(pngStruct, pngInfo, PNG_INFO_tRNS);

            //Prepare the data for storage and GPU upload.
            //Apply the color palette:
            if (colorType == PNG_COLOR_TYPE_PALETTE)
                png_set_palette_to_rgb(pngStruct);
            //If using RGB-based color and we have separate alpha info, inject that alpha info.
            if (hasSeparateAlpha)
                png_set_tRNS_to_alpha(pngStruct);
            //Texture formats less than 1 byte per pixel aren't allowed by OpenGL.
            if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
            {
                png_set_expand_gray_1_2_4_to_8(pngStruct);
                bitDepth = 8;
            }
            //16-bit channel data is stored big-endian.
            //If our machine is little-endian, swap the bytes.
            if (bitDepth == 16 && IsPlatformLittleEndian())
                png_set_swap(pngStruct);

            //TODO: If the data is marked as sRGB coming in, override the resulting Texture2D format (with the permission of a flag).
            uint32_t bytesPerPixel;
            switch (colorType)
            {
                case PNG_COLOR_TYPE_GRAY:
                    pixelDataChannels = PixelIOChannels::Red;
                    bytesPerPixel = bitDepth / 8;
                break;
                case PNG_COLOR_TYPE_GRAY_ALPHA:
                    pixelDataChannels = PixelIOChannels::RG;
                    bytesPerPixel = (bitDepth * 2) / 8;
                break;
                case PNG_COLOR_TYPE_RGB_ALPHA:
                    pixelDataChannels = PixelIOChannels::RGBA;
                    bytesPerPixel = (bitDepth * 4) / 8;
                break;

                case PNG_COLOR_TYPE_RGB:
                case PNG_COLOR_TYPE_PALETTE:
                    if (hasSeparateAlpha)
                    {
                        pixelDataChannels = PixelIOChannels::RGBA;
                        bytesPerPixel = (bitDepth * 4) / 8;
                    }
                    else
                    {
                        pixelDataChannels = PixelIOChannels::RGB;
                        bytesPerPixel = (bitDepth * 3) / 8;
                    }
                break;

                default:
                    std::string errorMsg = "Unknown LibPNG color type ";
                    errorMsg += std::to_string(colorType) + " (" + IO::ToHex(colorType) + ")";
                    BP_ASSERT(false, errorMsg.c_str());
                    return false;
            }
            switch (bitDepth)
            {
                case 8:
                    pixelDataType = PixelIOTypes::UInt8;
                    break;
                case 16:
                    pixelDataType = PixelIOTypes::UInt16;
                    break;

                default:
                    std::string errorMsg = "Unexpected LibPNG channel bit depth: ";
                    errorMsg += std::to_string(bitDepth);
                    BP_ASSERT(false, errorMsg.c_str());
                    return false;
            }

            //Read the data row by row.
            uint_fast32_t rowByteSize = bytesPerPixel * pixelSize.x;
            pixelData.resize((size_t)pixelSize.y * rowByteSize);
            //If the PNG is interlaced, we actually have to read the rows multiple times.
            auto nPasses = png_set_interlace_handling(pngStruct);
            for (int passI = 0; passI < nPasses; ++passI)
                for (uint32_t y = 0; y < pixelSize.y; ++y)
                {
                    //PNG row order is opposite from OpenGL row order.
                    uint32_t outY = pixelSize.y - 1 - y;
                    png_read_row(pngStruct, (png_bytep)(&pixelData[(size_t)outY * rowByteSize]), nullptr);
                }
        } break;

        case ImageLoaderFormats::JPEG: {
            auto jpeg = FindTurboJPEG();
            //TODO: Implement. https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/master/libjpeg.txt
        } break;

        default:
            std::string errMsg = "Unknown ImageLoaderFormats::";
            errMsg += GetFormat()->_to_string();
            BP_ASSERT(false, errMsg.c_str());
            return false;
    }

    return true;
}
bool ImageLoader::Create()
{
    Output.emplace(pixelSize, PixelFormat, NMips, Sampling, Swizzling);

    if (Output->GetFormat().IsDepthOnly())
    {
        BP_ASSERT(pixelDataChannels == +PixelIOChannels::Red,
                  "If loading a depth texture, the pixel data must be single-channel");
        Output->Set_Depth(pixelData.data(), pixelDataType);
    }
    else
    {
        BP_ASSERT(!Output->GetFormat().IsDepthStencil(),
                  "Can't create a stencil or depth/stencil texture from a PNG");
        Output->Set_Color(pixelData.data(), pixelDataChannels, pixelDataType);
    }

    return true;
}