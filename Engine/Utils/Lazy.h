#pragma once

#include "BPAssert.h"

//The Lazy<> struct allows for lazy instantiation of a class the first time it's needed.

namespace Bplus
{
    //An object that isn't actually constructed until it is needed.
    template<typename T>
    struct Lazy
    {
    public:
        using Inner_t = T;


        bool IsCreated() const { return isCreated; }

        template<typename... Args>
        void Create(Args&&... args)
        {
            BPAssert(!isCreated, "Already created this!");
            isCreated = true;
            new (&itemBytes[0]) T(std::forward<Args>(args)...);
        }

        
        //Gets the underlying data, creating it with the default constructor if necessary.
        const T& Get() const
        {
            //If not created already, the instance should be created now.
            //However, this operation is supposed to be a const op that returns a const instance,
            //    so we can't avoid some const casting.
            if (!isCreated)
                const_cast<Lazy<T>*>(this)->Create();

            return Cast();
        }
        T& Get()
        {
            return const_cast<T&>(const_cast<const Lazy<T>*>(this)->Get());
        }

        //Gets the underlying data, assuming it's already been created.
        const T& Cast() const
        {
            BPAssert(isCreated, "Accessed before creation");
            return PlainCast();
        }
        //Gets the underlying data, assuming it's already been created.
        T& Cast()
        {
            BPAssert(isCreated, "Accessed before creation");
            return PlainCast();
        }

        //Makes this object un-instantiated again.
        //If it was never instantiated in the first place, nothing happens.
        void Clear()
        {
            if (isCreated)
            {
                isCreated = false;
                PlainCast().~T();
            }
        }
        

        #pragma region Constructor, destructor, copy, move

        Lazy() { }
        ~Lazy()
        {
            if (isCreated)
                PlainCast().~T();
            isCreated = false;
        }

        Lazy(const T& cpy) : isCreated(true) { Create(cpy); }
        Lazy(const Lazy<T>& cpy)
            : isCreated(cpy.isCreated)
        {
            if (cpy.isCreated)
                Create(cpy.PlainCast());
        }
        Lazy& operator=(const Lazy<T>& cpy)
        {
            //If the new copy is empty, just destroy this instance.
            if (!cpy.isCreated)
            {
                if (isCreated)
                    PlainCast().~T();
            }
            //Otherwise, call into the assignment operator or constructor as necessary.
            else
            {
                if (isCreated)
                    PlainCast() = cpy.PlainCast();
                else
                    new(&PlainCast()) T(cpy.PlainCast());
            }

            isCreated = cpy.isCreated;
            return *this;
        }
        Lazy& operator=(const T& cpy)
        {
            if (isCreated)
                PlainCast() = cpy;
            else
                new(&PlainCast()) T(cpy);

            isCreated = true;
            return *this;
        }
        
        Lazy(T&& src) : isCreated(true) { Create(std::move(src)); }
        Lazy(Lazy<T>&& src)
            : isCreated(src.isCreated)
        {
            if (src.isCreated)
                Create(std::move(src.PlainCast()));
        }
        Lazy& operator=(Lazy<T>&& src)
        {
            //If the new value is empty, just destroy this instance.
            if (!src.isCreated)
            {
                if (isCreated)
                    PlainCast().~T();
            }
            //Otherwise, call into the assignment operator or constructor as necessary.
            else
            {
                if (isCreated)
                    PlainCast() = std::move(src.PlainCast());
                else
                    new(&PlainCast()) T(std::move(src.PlainCast()));
            }

            isCreated = src.isCreated;
            return *this;
        }
        Lazy& operator=(T&& cpy)
        {
            if (isCreated)
                PlainCast() = std::move(cpy);
            else
                new(&PlainCast()) T(std::move(cpy));

            isCreated = true;
            return *this;
        }

        #pragma endregion


    private:
        mutable bool isCreated = false;
        std::byte itemBytes[sizeof(T)];

        const T& PlainCast() const { return *(const T*)(&itemBytes[0]); }
              T& PlainCast()       { return *(      T*)(&itemBytes[0]); }
    };
}