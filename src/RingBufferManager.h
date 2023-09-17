// Copyright RedFox Studio 2022

#pragma once

#include "asserts.h"

namespace Fox
{

struct RingBufferManager
{
  private:
    inline bool _shouldWrapAround(uint32_t index) { return index >= MaxSize; };
    inline bool _isInsideRange(uint32_t start, uint32_t length) const { return start < MaxSize && start + length <= MaxSize; };

  public:
    RingBufferManager(uint32_t maxSize, unsigned char* mappedMemory) : MaxSize(maxSize), Mapped(mappedMemory) {}

    // Must receive the same number of pop with the same length as the number of time Push was called
    void Pop(uint32_t length)
    {
        check(length, MaxSize);
#if defined(_DEBUG)
        if (!Full && Tail != Head)
            {
                if (Tail < Head)
                    {
                        check(Tail + length <= Head);
                    }
            }
#endif
        const bool fit = Tail + length <= MaxSize;
        if (fit)
            {
                Tail += length;
            }
        else
            {
                // Increase or wrap around
                Tail = _shouldWrapAround(Tail + length) ? Tail = length : Tail + length;
            }

        if (Tail == MaxSize)
            {
                Tail = 0;
            }

        check(Tail >= 0 && Tail <= MaxSize);
        Full = false; // When Pop is called it means the we're not full anymore
    }

    // Returns the actual space that we can allocate with push. Can be less than available space
    uint32_t Capacity() const
    {
        if (Full)
            {
                return 0;
            }
        if (Head == Tail)
            {
                return MaxSize;
            }

        if (Head < Tail)
            {
                return Tail - Head;
            }
        else
            {
                return MaxSize - Head;
            }
    }

    /*Returns the size of actual used memory*/
    uint32_t Size() const
    {
        if (Full)
            {
                return MaxSize;
            }
        else
            {
                if (Head >= Tail)
                    {
                        return Head - Tail;
                    }
                else
                    {
                        return MaxSize - (Tail - Head);
                    }
            }
    }

    /*If data is nullptr no copy is performed*/
    uint32_t Push(void* data, uint32_t length)
    {
        // Must not be full
        check(!Full);
        // Allocation can be more than max size
        check(length <= MaxSize);
        // Check capacity
        check(Capacity() >= length);
#if defined(_DEBUG)
        if (Head != Tail)
            {
                if (Head < Tail)
                    {
                        // If head < tail cannot go beyond tail
                        check(Head + length <= Tail);
                    }
                // else if (_shouldWrapAround(Head + length))
                //     {
                //         // If head > tail and wrap arounds can't go beyond tail
                //         check(length <= Tail);//no Capacity left
                //     }
            }
#endif

        const bool fit = Head + length <= MaxSize;
        if (fit)
            {
                Head += length;
            }
        else
            {
                // Increase or wrap around
                Head = _shouldWrapAround(Head + length) ? Head = length : Head + length;
            }
        // Mark as full if are equal
        if (Head == Tail)
            {
                Full = true;
            }

        const uint32_t dataStart = Head - length;
        check(_isInsideRange(dataStart, length));
        if (data != nullptr)
            {
                unsigned char* addressWithOffset = static_cast<unsigned char*>(Mapped) + dataStart;
                memcpy(addressWithOffset, data, length);
            }

        // Mark full and wrap around otherwise capacity is 0
        if (Head == MaxSize)
            {
                if (Tail == 0)
                    {
                        Full = true;
                    }
                Head = 0;
            }

        // Return data location
        return dataStart;
    }

    const uint32_t MaxSize{};
    unsigned char* Mapped{};
    uint32_t       Head{};//Goes from 0 To MaxSize incluse
    uint32_t       Tail{};//Goes from 0 To MaxSize incluse
    bool           Full{ false }; // This is used when the Head==Tail, if true it means that no available space is left
};
}