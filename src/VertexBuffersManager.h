// Copyright RedFox Studio 2022

#pragma once

#include "Core/ManagerInterface.h"
#include "Core/math/AABox.h"
#include "Core/utils/DGuid.h"
#include "Image/ImageLoader.h"
#include "RenderInterface/vulkan/RIFactoryVk.h"
#include <memory>

namespace Fox
{



class CVertexBufferManager : public CManagerInterface
{
  public:
    void Initialize();
    void Deinitialize();
    void Tick() override{};

    std::shared_ptr<DRIBuffer> CreateVertexBuffer(const DGUID& guid, void* data, size_t length);

  private:
    RIFactoryVk*                                          _factory;
    std::unordered_map<DGUID, std::shared_ptr<DRIBuffer>> _guidToVertexBuffer;
};

}