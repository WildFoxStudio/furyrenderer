// Copyright RedFox Studio 2022

#pragma once

#include "Core/utils/debug.h"

// Design it like a ring buffer
struct RingBufferManager
{
    RingBufferManager(uint32_t maxSize, unsigned char* mappedMemory) : MaxSize(maxSize), Mapped(mappedMemory) {}

    // Free space for the head to move forward
    void SetTail(uint32_t tail)
    {
        check(tail <= MaxSize);
#if _DEBUG
        // We cannot move tail in front of head manually if it was behing
        if (Tail <= Head)
            {
                check(tail <= Head);
            }
        else // if tail was in ahead of head
            {
                // Tail must not decrease, meaning it went past Head or it should be less than head
                check(tail > Tail || tail < Head);
            }
#endif
        Tail = tail;
        // If equal then reset to beginning
        if (Tail == Head)
            {
                Tail = 0;
                Head = 0;
            }
    }

    uint32_t GetHead() const { return Head; };
    uint32_t GetTail() const { return Tail; };

    bool DoesFit(uint32_t length) const
    {
        if (length > MaxSize)
            return false;

        if (Head >= Tail)
            {
                // Head is ahead of or at the same position as the tail
                if (Head + length > MaxSize) // If wrap around
                    {
                        return length <= Tail;
                    }
            }
        else
            {
                // Head is behind the tail, the buffer wraps around
                return (Tail - Head);
            }

        return !Full;
    }

    uint32_t Copy(void* data, uint32_t length)
    {
        check(DoesFit(length));
        check(length <= MaxSize);

        if (Head + length > MaxSize) // we cannot exceed the end of the buffer so we wrap around head
            { // This space is free and lost in the buffer, so it's better to allocate bigger data first and smaller second
                Head = 0;
            }
#if _DEBUG
        if (Head < Tail)
            {
                check(Head + length <= Tail);
            }
#endif

        const uint32_t previousHead      = Head;
        unsigned char* addressWithOffset = static_cast<unsigned char*>(Mapped) + Head;
        memcpy(addressWithOffset, data, length);

        // Increment tail
        Head += length;
        // If head an tail are the same it means that ring buffer is full
        if (Head == Tail)
            {
                Full = true;
            }

        return previousHead;
    }

    /* The available space is only an estimation, use DoesFit to check if can Copy*/
    uint32_t AvailableSpace() const
    {
        if (Full)
            return 0;

        if (Head >= Tail)
            {
                // Head is ahead of or at the same position as the tail
                return (MaxSize - (Head - Tail));
            }
        else
            {
                // Head is behind the tail, the buffer wraps around
                return (Tail - Head);
            }
    }

    const uint32_t MaxSize{};
    unsigned char* Mapped{};
    uint32_t       Head{};
    uint32_t       Tail{};
    bool           Full{ false }; // This is used when the Head==Tail, if true it means that no available space is left
};