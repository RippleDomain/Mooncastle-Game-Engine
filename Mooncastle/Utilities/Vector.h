#pragma once

#include "CommonHeaders.h"

namespace mooncastle::utl
{
    /*A vector class similar to std::vector with basic functionality in place.
    The user can specify in the template argument whether they want the
    elements' destructor to be called when it gets removed or while
    clearing/destroying the vector.*/
    template<typename T, bool destruct = true>
    class vector
    {
    public:
        //Default constructor. Does not allocate memory.
        vector() = default;

        //Constructor resizes the vector and initializes "count" items.
        constexpr explicit vector(u64 count)
        {
            resize(count);
        }

        //Constructor resizes the vector and initializes "count" items using 'value'.
        constexpr explicit vector(u64 count, const T& value)
        {
            resize(count, value);
        }

        template<typename it, typename = std::enable_if_t<std::_Is_iterator_v<it>>>
        constexpr explicit vector(it first, it last)
        {
            for (; first != last; ++first)
            {
                emplace_back(*first);
            }
        }

        //Copy-constructor. Constructs by copying another vector. The items in the copied vector must be copyable.
        constexpr vector(const vector& o)
        {
            *this = o;
        }

        //Move-constructor. Constructs by moving another vector. The original vector will be empty after move.
        constexpr vector(vector&& o) : currentCapacity{ o.currentCapacity }, currentSize{ o.currentSize }, currentData{ o.currentData }
        {
            o.reset();
        }

        //Copy-assignment operator. Clears this vector and copies items from another vector. The items must be copyable.
        constexpr vector& operator=(const vector& o)
        {
            assert(this != std::addressof(o));

            if (this != std::addressof(o))
            {
                clear();
                reserve(o.currentSize);

                for (auto& item : o)
                {
                    emplace_back(item);
                }

                assert(currentSize == o.currentSize);
            }

            return *this;
        }

        //Move-assignment operator. Frees all resources in this vector and  moves the other vector into this one.
        constexpr vector& operator=(vector&& o)
        {
            assert(this != std::addressof(o));

            if (this != std::addressof(o))
            {
                destroy();
                move(o);
            }

            return *this;
        }

        //Destructs the vector and its items as specified in template argument.
        ~vector() 
        { 
            destroy();
        }

        //Inserts an item at the end of the vector by copying 'value'.
        constexpr void push_back(const T& value)
        {
            emplace_back(value);
        }

        //Inserts an item at the end of the vector by moving 'value'.
        constexpr void push_back(T&& value)
        {
            emplace_back(std::move(value));
        }

        //Copy- or move-constructs an item at the end of the vector.
        template<typename... params>
        constexpr decltype(auto) emplace_back(params&&... p)
        {
            if (currentSize == currentCapacity)
            {
                reserve(((currentCapacity + 1) * 3) >> 1); //Reserve 50% more.
            }

            assert(currentSize < currentCapacity);

            T *const item{ new (std::addressof(currentData[currentSize])) T(std::forward<params>(p)...) };
            ++currentSize;

            return *item;
        }

        //Resizes the vector and initializes new items with their default value.
        constexpr void resize(u64 newSize)
        {
            static_assert(std::is_default_constructible_v<T>, "Type must be default-constructible.");

            if (newSize > currentSize)
            {
                reserve(newSize);

                while (currentSize < newSize)
                {
                    emplace_back();
                }
            }
            else if (newSize < currentSize)
            {
                if constexpr (destruct)
                {
                    destroyRange(newSize, currentSize);
                }

                currentSize = newSize;
            }

            //Do nothing if newSize == size.
            assert(newSize == currentSize);
        }

        //Resizes the vector and initializes new items by copying "value".
        constexpr void resize(u64 newSize, const T& value)
        {
            static_assert(std::is_copy_constructible_v<T>, "Type must be copy-constructible.");

            if (newSize > currentSize)
            {
                reserve(newSize);

                while (currentSize < newSize)
                {
                    emplaceBack(value);
                }
            }
            else if (newSize < currentSize)
            {
                if constexpr (destruct)
                {
                    destroyRange(newSize, currentSize);
                }

                currentSize = newSize;
            }

            //Do nothing if newSize == size.
            assert(newSize == currentSize);
        }

        //Allocates memory to contain the specified number of items.
        constexpr void reserve(u64 newCapacity)
        {
            if (newCapacity > currentCapacity)
            {
                //realloc() will automatically copy the data in the buffer if a new region of memory is allocated.
                void* newBuffer{ realloc(currentData, newCapacity * sizeof(T)) };

                assert(newBuffer);

                if (newBuffer)
                {
                    currentData = static_cast<T*>(newBuffer);
                    currentCapacity = newCapacity;
                }
            }
        }

        //Removes the item at specified index.
        constexpr T* const erase(u64 index)
        {
            assert(currentData && index < currentSize);
            return erase(std::addressof(currentData[index]));
        }

        //Removes the item at specified location.
        constexpr T* const erase(T* const item)
        {
            assert(currentData && item >= std::addressof(currentData[0]) && item < std::addressof(currentData[currentSize]));

            if constexpr (destruct) item->~T();
            --currentSize;

            if (item < std::addressof(currentData[currentSize]))
            {
                memcpy(item, item + 1, (std::addressof(currentData[currentSize]) - item) * sizeof(T));
            }

            return item;
        }

        //Same as erase() but faster because it just copies the last item.
        constexpr T* const erase_unordered(u64 index)
        {
            assert(currentData && index < currentSize);
            return erase_unordered(std::addressof(currentData[index]));
        }

        //Same as erase() but faster because it just copies the last item.
        constexpr T* const erase_unordered(T* const item)
        {
            assert(currentData && item >= std::addressof(currentData[0]) && item < std::addressof(currentData[currentSize]));

            if constexpr (destruct) item->~T();
            --currentSize;

            if (item < std::addressof(currentData[currentSize]))
            {
                memcpy(item, std::addressof(currentData[currentSize]), sizeof(T));
            }

            return item;
        }

        //Clears the vector and destroys items as specified in template argument.
        constexpr void clear()
        {
            if constexpr (destruct)
            {
                destroyRange(0, currentSize);
            }

            currentSize = 0;
        }

        //Swaps two vectors.
        constexpr void swap(vector& o)
        {
            if (this != std::addressof(o))
            {
                auto temp(std::move(o));
                o.move(*this);
                move(temp);
            }
        }

        //Pointer to the start of the data.
        [[nodiscard]] constexpr T* data()
        {
            return currentData;
        }

        //Pointer to the start of the data.
        [[nodiscard]] constexpr T*const data()const
        {
            return currentData;
        }

        //Returns true if the vector is empty.
        [[nodiscard]] constexpr bool empty()const
        {
            return currentSize == 0;
        }

        //Returns the current number of items in the vector.
        [[nodiscard]] constexpr u64 size()const
        {
            return currentSize;
        }

        //Returns the current capacity of the vector.
        [[nodiscard]] constexpr u64 capacity()const
        {
            return currentCapacity;
        }

        //Indexing operator. Returns a reference to the item at the specified index within the vector.
        [[nodiscard]] constexpr T& operator[](u64 index)
        {
            assert(currentData && index < currentSize);
            return currentData[index];
        }

        //Indexing operator. Returns a constant reference to the item at the specified index within the vector.
        [[nodiscard]] constexpr const T& operator[](u64 index)const
        {
            assert(currentData && index < currentSize);
            return currentData[index];
        }

        //Returns a reference to the first item in the vector. Will fault the application if it is called when the vector is empty.
        [[nodiscard]] constexpr T& front()
        {
            assert(currentData && currentSize);
            return currentData[0];
        }

        //Returns a reference to the first item in the vector. Will fault the application if it is called when the vector is empty.
        [[nodiscard]] constexpr const T& front()const
        {
            assert(currentData && currentSize);
            return currentData[0];
        }

        //Returns a reference to the last item in the vector. Will cause a crash if it is called when the vector is empty.
        [[nodiscard]] constexpr T& back()
        {
            assert(currentData && currentSize);
            return currentData[currentSize - 1];
        }

        //Returns a reference to the last item in the vector. Will cause a crash if it is called when the vector is empty.
        [[nodiscard]] constexpr const T& back()const
        {
            assert(currentData && currentSize);
            return currentData[currentSize - 1];
        }

        //Returns a pointer to the first item. Returns null when the vector is empty.
        [[nodiscard]] constexpr T* begin()
        {
            assert(currentData);
            return std::addressof(currentData[0]);
        }

        //Returns a constant pointer to the first item. Returns null when the vector is empty.
        [[nodiscard]] constexpr const T* begin()const
        {
            assert(currentData);
            return std::addressof(currentData[0]);
        }

        //Returns a pointer to the first item. Returns null when the vector is empty.
        [[nodiscard]] constexpr T* end()
        {
            assert(!(currentData == nullptr && currentSize > 0));
            return std::addressof(currentData[currentSize]);
        }

        //Returns a constant pointer to the last item. Returns null when the vector is empty.
        [[nodiscard]] constexpr const T* end()const
        {
            assert(!(currentData == nullptr && currentSize > 0));
            return std::addressof(currentData[currentSize]);
        }

    private:
        constexpr void move(vector& o)
        {
            currentCapacity = o.currentCapacity;
            currentSize = o.currentSize;
            currentData = o.currentData;
            o.reset();
        }

        constexpr void reset()
        {
            currentCapacity = 0;
            currentSize = 0;
            currentData = nullptr;
        }

        constexpr void destroyRange(u64 first, u64 last)
        {
            assert(destruct);
            assert(first <= currentSize && last <= currentSize && first <= last);

            if (currentData)
            {
                for (; first != last; ++first)
                {
                    currentData[first].~T();
                }
            }
        }

        constexpr void destroy()
        {
            assert([&] { return currentCapacity ? currentData != nullptr : currentData == nullptr; }());
            clear();
            currentCapacity = 0;
            if (currentData) free(currentData);
            currentData = nullptr;
        }

        u64 currentCapacity{ 0 };
        u64 currentSize{ 0 };
        T* currentData{ nullptr };
    };
}