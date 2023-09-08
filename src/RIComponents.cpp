#include "RIComponents.h"

#include "RenderInterface/vulkan/UtilsVK.h"

namespace Fox
{
RIFenceVk::RIFenceVk(bool signaleted) { _signaled = signaleted; }

void
RIFenceVk::WaitForSignal(uint64_t timeoutNanoseconds)
{
    const VkResult result = vkWaitForFences(Device_DEPRECATED, 1, &Fence, VK_TRUE /*Wait all*/, timeoutNanoseconds);
    if (VKFAILED(result))
        {
            throw std::runtime_error(VkErrorString(result));
        }
    else
        {
            _signaled = true;
        }
}
void
RIFenceVk::Reset()
{
    vkResetFences(Device_DEPRECATED, 1, &Fence);
    _signaled = false;
};


}