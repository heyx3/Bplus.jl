#pragma once

#include <filesystem>
#include <unordered_map>

#include "../IO.h"
#include "../GL/Textures/TextureD.hpp"

#include "Loader.h"


namespace Bplus::Assets
{
    #pragma region Base class ContentFolderLoader

    //Loads an asset from the B+ 'content' folder.
    template<typename Asset_t>
    class ContentFolderLoader : public Loader<Asset_t>
    {
    public:

        //The path that the data file is relative to.
        std::string BasePath = BPLUS_CONTENT_FOLDER;

        std::string GetFilePath() const
        {
            auto asPath = fs::path(BasePath);
            asPath = asPath / Path;
            return asPath.string();
        }

        virtual bool IsPathValid() const override
        {
            return std::filesystem::exists(GetFilePath());
        }
        virtual bool Retrieve() sealed
        {
            std::vector<std::byte> diskData;
            if (!Bplus::IO::LoadEntireFile(GetFilePath(), diskData))
                return false;

            return ProcessAfterRetrieve(std::move(diskData));
        }


    protected:

        //Does any extra processing at the end of Retrieve,
        //    after the data was successfully retrieved from disk.
        virtual bool ProcessAfterRetrieve(std::vector<std::byte>&& diskData) = 0;
    };

    #pragma endregion


    #pragma region Image loader

    BETTER_ENUM(ImageLoaderFormats, uint8_t,
        PNG, JPEG, BMP
        //TODO: More formats.
    );
    
    //A lookup of the default file extensions for each type of ImageLoader format.
    //The extension strings are NOT preceded by a dot.
    inline const auto& GetDefaultExtensions()
    {
        static auto value = std::unordered_map<ImageLoaderFormats, std::vector<const char*>>{
            { ImageLoaderFormats::PNG, { "png" } },
            { ImageLoaderFormats::JPEG, { "jpg", "jpeg", "jpe" } },
            { ImageLoaderFormats::BMP, { "bmp" } }
        };
        return value;
    }

    //Guesses an image format from its extension.
    //The extension should NOT include the period.
    //Returns nothing if the extension doesn't match any format.
    BP_API std::optional<ImageLoaderFormats> GuessImageFormat(const std::string& extension);


    //Loads an image file, detecting the format based on its extension.
    class BP_API ImageLoader : public ContentFolderLoader<GL::Textures::Texture2D>
    {
    public:

        //If true, the texture is loaded as a one-component depth texture.
        bool LoadAsDepth = false;
        //If set, the loader will assume the file is in this format
        //    instead of determining the format from the extension.
        std::optional<ImageLoaderFormats> ForcedFormat = std::nullopt;

        //The number of mip levels.
        // 0 means that mips are calculated automatically.
        // 1 effectively disables mips.
        Bplus::GL::Textures::uint_mipLevel_t NMips = 0;
        //The default sampler settings for the texture.
        Bplus::GL::Textures::Sampler<2> Sampling;
        //The swizzling for this texture.
        Bplus::GL::Textures::SwizzleRGBA Swizzling = Bplus::GL::Textures::DefaultSwizzling();


        virtual bool IsPathValid() const override;
        virtual bool ProcessAfterRetrieve(std::vector<std::byte>&& diskData);
        virtual bool Create() override;

        //Gets the format this loader believes it should use, based on the settings.
        std::optional<ImageLoaderFormats> GetFormat() const;

    private:

        glm::uvec2 pixelSize;
        std::vector<std::byte> pixelData;
        Textures::Format pixelFormat;
    };

    #pragma endregion
}