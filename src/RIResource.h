// Copyright RedFox Studio 2022

#pragma once

#include "Core/memory/AtomicCounter.h"
#include <vector>

namespace Fox
{
struct RIResource : public CAtomicCounter
{
    RIResource() = default;
    RIResource(RIResource&& other) noexcept : BoundResources(other.BoundResources){};

    std::vector<RIResource*> BoundResources;

    inline void AcquireResource(RIResource* resource)
    {
        BoundResources.push_back(resource);
        resource->IncreaseCounter();
    };

    inline void ReleaseResources()
    {

        for (RIResource* c : BoundResources)
            {
                c->DecreaseCounter();
                c->ReleaseResources();
            }
        BoundResources.clear();
    };
};

}