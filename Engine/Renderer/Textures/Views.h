#pragma once

#include <numeric>

#include "Sampler.h"
#include "../../Math/Box.hpp"


namespace Bplus::GL::Textures
{
    class Texture;


    //A view of the texture, potentially with custom sampler settings.
    class BP_API TextureView
    {
    private:

        friend class Bplus::GL::Textures::Texture;

        TextureView(const Texture* src);
        TextureView(const Texture* src,
                    TextureMinFilters minFilter, TextureMagFilters magFilter,
                    WrapModes* wrappingPerAxis, glm::length_t samplerDimensions);
        TextureView(const Texture* src,
                    TextureMinFilters minFilter, TextureMagFilters magFilter,
                    WrapModes* wrappingPerAxis, glm::length_t samplerDimensions,
                    Math::IntervalF mipClampRange, float mipOffset);
        //TODO: Add overload for Comparison functions.

        ~TextureView();


    public:

        //The per-axis texture wrapping.
        //May not use all 3 dimensions, depending on what kind of texture
        //    it was created with.
        const std::array<WrapModes, 3> WrapParams3D;

        const TextureMinFilters MinFilter;
        const TextureMagFilters MagFilter;
        
        const Math::IntervalF MipClampRange;
        const float MipOffset;

        



        //Marks a new source that wants this view to stay active.
        //Must be paired with a call to "Deactivate()".
        void Activate();
        //Marks the end of a desire for this view to stay active.
        //Must be paired with a call to "Activate"().
        void Deactivate();



    private:
        
        //If true, OpenGL is currently allowing this handle to be used.
        //The standard refers to this as being "resident".
        bool isActive;

        //The number of sources that want this handle to be active.
        //Acts as reference-counter that deactivates this handle when it hits 0.
        uint_fast32_t activeCount = 0;

        const Texture* texture;
        OglPtr::Sampler samplerGlPtr = OglPtr::Sampler::Null();
        OglPtr::View viewGlPtr;
    };



    //Keeps a certain TextureView or ImageView active for as long as this object is alive.
    //Must not stay alive longer than the view itself.
    template<typename View_t>
    class BP_API ViewHolder
    {
    public:

        ViewHolder(View_t* _view) : view(_view) { view->Activate(); }
        ~ViewHolder() { view->Deactivate(); }

        //Copying simply creates another hold on the view.
        //Views use reference-counting, so the cost of this is negligible.
        ViewHolder(const ViewHolder<View_t>& cpy) : view(cpy.view) { view->Activate(); }
        ViewHolder& operator=(const ViewHolder<View_t>& cpy)
        {
            //Deconstruct this instance, then use the copy constructor.
            this->~ViewHolder();
            new (this) ViewHolder(cpy);

            return *this;
        }

        //Moving can just perform a copy for simplicity.
        ViewHolder(ViewHolder<View_t>&& from) : ViewHolder(from) { }
        ViewHolder& operator=(ViewHolder<View_t>&& from) { return operator=(from); }

    private:

        View_t* view;
    };
}