#pragma once

#include <filesystem>

#include "../IO.h"

#include "Loader.h"


namespace Bplus::Assets
{
    //Loads an asset from the B+ 'content' folder.
    template<typename Asset_t>
    class BP_API ContentFolderLoader : public Loader<Asset_t>
    {
        std::string GetFilePath() const
        {
            return std::string(BPLUS_CONTENT_FOLDER) + "/" + Path;
        }

    protected:


        virtual bool IsPathValid() override
        {
            return std::filesystem::exists(GetFilePath());
        }
        
        virtual bool Retrieve() sealed
        {
            std::vector<std::byte> diskData;
            if (!Bplus::IO::LoadEntireFile(GetFilePath(), diskData))
                return false;

            return ProcessAfterRetrieve(diskData);
        }

        //Does any extra processing at the end of Retrieve,
        //    after the data was successfully retrieved from disk.
        virtual bool ProcessAfterRetrieve(std::vector<std::byte>& diskData) { return true; }
    };
}