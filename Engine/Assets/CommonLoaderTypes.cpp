#include "CommonLoaderTypes.h"

#include "../IO.h"

using namespace Bplus;
using namespace Bplus::Assets;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


namespace {
    #pragma region RAII wrappers for image decompressors

    struct TurboJpegDecompressor
    {
        jpeg_decompress_struct Data;
        TurboJpegDecompressor()
        {
            jpeg_create_decompress(&Data);
        }
        ~TurboJpegDecompressor()
        {
            jpeg_destroy_decompress(&Data);
        }
    };

    struct LibPngDecompressor
    {
        png_structp Struct;
        png_infop Info;
        LibPngDecompressor(bool& out_DidFail)
        {
            out_DidFail = true;

            Struct = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                            nullptr, nullptr, nullptr);
            if (Struct == nullptr)
            {
                BP_ASSERT(false, "Unable to initialize a libPNG reader");
                return;
            }

            Info = png_create_info_struct(Struct);
            if (Info == nullptr)
            {
                png_destroy_read_struct(&Struct, nullptr, nullptr);
                Struct = nullptr;

                BP_ASSERT(false, "Unable to initialize a libPNG 'info' struct for reading");
                return;
            }

            out_DidFail = false;
        }
        ~LibPngDecompressor()
        {
            if (Struct != nullptr)
            {
                png_destroy_read_struct(&Struct,
                                        Info ? (&Info) : nullptr,
                                        nullptr);
            }
        }
    };

    #pragma endregion

    thread_local std::optional<TurboJpegDecompressor> turboJPEGThreadInstance = std::nullopt;
    thread_local std::optional<LibPngDecompressor> libPNGThreadInstance = std::nullopt;

    auto* FindTurboJPEG()
    {
        //Refer to: https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/master/example.txt
        //          https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/master/libjpeg.txt

        auto& threadReference = turboJPEGThreadInstance;

        if (!threadReference.has_value())
            threadReference.emplace();

        return &threadReference->Data;
    }
    auto FindLibPNG()
    {
        //Refer to: https://github.com/glennrp/libpng/blob/libpng16/libpng-manual.txt

        auto& threadReference = libPNGThreadInstance;

        if (!threadReference.has_value())
        {
            bool failed = false;
            threadReference.emplace(failed);
            if (failed)
            {
                threadReference.reset();
                BP_ASSERT(false, "TurboPNG didn't initialize properly");
            }
        }

        return std::make_tuple(threadReference->Struct, threadReference->Info);
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
                    BP_ASSERT_STR(false,
                                  "Unknown LibPNG color type: " +
                                     std::to_string(colorType) +
                                     " (" + IO::ToHex(colorType) + ")");
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
                    BP_ASSERT_STR(false,
                                  "Unexpected LibPNG channel bit depth: " + std::to_string(bitDepth));
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
            //Start reading the JPEG.
            auto jpeg = FindTurboJPEG();
            jpeg_mem_src(jpeg, (const unsigned char*)diskData.data(), (unsigned long)diskData.size());
            jpeg_read_header(jpeg, true);
            jpeg_start_decompress(jpeg);

            //Prepare the output pixel buffer.
            pixelSize = { jpeg->output_width, jpeg->output_height };
            auto rowByteSize = (size_t)pixelSize.x * jpeg->output_components;
            pixelData.resize(rowByteSize * pixelSize.y);

            //Read the data row by row.
            //JPEG standard is top-to-bottom, while OpenGL is bottom-to-top,
            //    so we have to invert the row order.
            //Fortunately, JPEG operates on each row individually
            //    by taking an array of row pointers, so it's easy to flip the data.
            std::vector<JSAMPROW> rowPtrs(pixelSize.y);
            for (uint32_t row = 0; row < pixelSize.y; ++row)
            {
                auto row_idx = pixelSize.y - row - 1;
                rowPtrs[row] = (JSAMPROW)(&pixelData[row_idx * rowByteSize]);
            }
            //We have to continuously read the rows in a loop
            //    until we're sure all of them have been read.
            while (jpeg->output_scanline < pixelSize.y)
                jpeg_read_scanlines(jpeg, rowPtrs.data(), pixelSize.y);

            jpeg_finish_decompress(jpeg);
        } break;

        default:
            BP_ASSERT_STR(false,
                          "Unknown ImageLoaderFormats::" +
                              GetFormat()->_to_string());
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