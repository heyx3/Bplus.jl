#include "CommonLoaderTypes.h"

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
    if (ForcedFormat.has_value())
        return ForcedFormat.value();
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
        case ImageLoaderFormats::BMP: {
            ebmp::BMP bmp;
            auto inStream = InputMemoryStream(diskData.data(), diskData.size());
            if (!bmp.ReadFromStream(inStream))
                return false;

            pixelFormat = SimpleFormat(FormatTypes::NormalizedUInt,
                                       SimpleFormatComponents::RGB,
                                       SimpleFormatBitDepths::B8);

            pixelData.resize(3 * bmp.TellWidth() * bmp.TellHeight());
            for (int y = 0; y < bmp.TellHeight(); ++y)
                for (int x = 0; x < bmp.TellWidth(); ++x)
                {
                    size_t offset = 3 * (x + (y * (size_t)bmp.TellWidth()));
                    auto pixel = bmp.GetPixelColumns()[x][y];
                    pixelData[offset] = (std::byte)pixel.Red;
                    pixelData[offset + 1] = (std::byte)pixel.Green;
                    pixelData[offset + 2] = (std::byte)pixel.Blue;
                }
        } break;
            
        case ImageLoaderFormats::PNG: {
        } break;

        case ImageLoaderFormats::JPEG: {
        } break;

        default:
            std::string errMsg = "Unknown ImageLoaderFormats::";
            errMsg += GetFormat()->_to_string();
            BP_ASSERT(false, errMsg.c_str());
            return false;
    }
}
bool ImageLoader::Create()
{
    Output.emplace(pixelSize, pixelFormat, NMips, Sampling, Swizzling);
}