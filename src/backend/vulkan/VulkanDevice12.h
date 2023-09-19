// Copyright RedFox Studio 2022

#pragma once

#include "RIResource.h"
#include "VulkanDevice11.h"

#include <volk.h>

#include <list>

namespace Fox
{

struct RICommandBuffer : public RIResource
{
    RICommandBuffer(VkCommandBuffer cmd) : Cmd(cmd){};
    VkCommandBuffer Cmd{};
};

/*Does not own the pool, must be freed by the vulkan device*/
struct RICommandPool
{
    explicit RICommandPool(VkDevice device, VkCommandPool pool) : _device(device), _poolManager(pool){};

    /*All the command buffers once reset go into initial state*/
    void Reset();

    /*The pointer must not be deleted, is manages by the pool*/
    RICommandBuffer* Allocate();

    // Move constructor
    RICommandPool(RICommandPool&& other) : _device(other._device), _poolManager(other._poolManager)
    {
        // Move the contents from 'other' to this object
        other._poolManager = nullptr;
    }

    // Delete copy constructor and copy assignment operator
    RICommandPool(const RICommandPool&)            = delete;
    RICommandPool& operator=(const RICommandPool&) = delete;

  private:
    VkDevice                   _device;
    VkCommandPool              _poolManager;
    std::list<RICommandBuffer> _cachedCmds; // pool of commands
    friend class RIVulkanDevice12;
};

struct RICommandPoolManager
{
    RICommandPoolManager(RICommandPool* pool) : _poolManager(std::move(pool)){};

    /*The pointer must not be deleted, is manages by the pool*/
    inline RICommandBuffer* Allocate()
    {
        if (_cmds.size() == 0)
            {
                _recorded.push_back(_poolManager->Allocate());
            }
        else
            {
                _recorded.push_back(_cmds.back());
                _cmds.pop_back();
            }
        return _recorded.back();
    };
    inline void Reset()
    {
        _poolManager->Reset();
        if (_recorded.size() > 0)
            {
                std::for_each(_recorded.begin(), _recorded.end(), [this](RICommandBuffer* cmd) { cmd->ReleaseResources(); });
                _cmds.insert(_cmds.end(), _recorded.begin(), _recorded.end());
                _recorded.clear();
            }
    }

    /*The pointers must not be deleted, are manages by the pool*/
    inline std::vector<RICommandBuffer*> GetRecorded() { return _recorded; }

    RICommandPool* GetCommandPool() const { return _poolManager; };

  private:
    RICommandPool*                _poolManager;
    std::vector<RICommandBuffer*> _cmds; // all allocated command buffers
    std::vector<RICommandBuffer*> _recorded; // Getted cmds after the reset method
};

class RIVulkanDevice12 : public RIVulkanDevice11
{
  public:
    /*don't delete pointer, is managed by a pool*/
    RICommandPool* CreateCommandPool();
    /*This is explicit deletion, will be deleted in destructor automatically*/
    void DestroyCommandPool(RICommandPool*);

    void SubmitToMainQueue(const std::vector<RICommandBuffer*>& cmds, const std::vector<VkSemaphore>& waitSemaphore, VkSemaphore finishSemaphore, VkFence fence);

  public:
    virtual ~RIVulkanDevice12();

    std::list<RICommandPool> _cachedPools;
};
}