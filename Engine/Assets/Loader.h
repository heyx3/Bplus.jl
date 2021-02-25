#pragma once

#include <optional>

#include "../Utils.h"


namespace Bplus::Assets
{
    //Loads an asset.
    //Asset loading is split into two phases:
    //    1. Retrieving
    //    2. Creating
    //Retrieving can theoretically be done on any thread (e.x. loading a file from disk),
    //    while Creating should be guaranteed to run on the "main" thread
    //    (e.x. the OpenGL context's thread if the asset is a shader).
    //Do NOT inherit from this class directly; inherit from Loader<T> instead.
    class BP_API LoaderBase
    {
    public:

        //The 'path' to the asset.
        //The exact meaning of this string is up to the loader.
        std::string Path;


        //Checks the path, so we can avoid trying to load the asset if the path isn't valid.
        virtual bool IsPathValid() = 0;

        //Executes the first step of the asset load, assuming IsPathValid() is true.
        //Returns whether it succeeded.
        virtual bool Retrieve() = 0;
        //Executes the second step of the asset load.
        //Returns whether it succeeded.
        virtual bool Create() = 0;
        

    protected:
        //An error/warning message once retrieval and/or creation is done.
        std::string ResultMsg;


        virtual ~LoaderBase() { }
    };


    //Loads a specific type of asset.
    template<typename Asset_t>
    class BP_API Loader : public LoaderBase
    {
        //The loaded asset.
        std::optional<Asset_t> Output;
    };
}