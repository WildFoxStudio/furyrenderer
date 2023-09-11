
#include "VulkanContext.h"

#include "UtilsVK.h"

#include <string>

namespace Fox
{

namespace Private
{
//@TODO can be unit tested
std::vector<const char*>
filterInclusive(const std::vector<const char*> source, const std::vector<const char*> included)
{
    std::vector<const char*> inclusive;
    std::copy_if(source.cbegin(), source.cend(), std::back_inserter(inclusive), [&included](const char* s1) {
        const auto found = std::find_if(included.begin(), included.end(), [s1](const char* s2) { return (strcmp(s1, s2) == 0); });
        return found != included.end();
    });

    return inclusive;
}
//@TODO can be unit tested
std::vector<const char*>
filterExclusive(const std::vector<const char*> source, const std::vector<const char*> excluded)
{
    std::vector<const char*> inclusive;
    std::copy_if(source.cbegin(), source.cend(), std::back_inserter(inclusive), [&excluded](const char* s1) {
        const auto found = std::find_if(excluded.begin(), excluded.end(), [s1](const char* s2) { return (strcmp(s1, s2) == 0); });
        return found == excluded.end();
    });

    return inclusive;
}

std::vector<const char*>
convertLayerPropertiesToNames(const std::vector<VkLayerProperties>& layers)
{
    std::vector<const char*> names;

    std::transform(layers.cbegin(), layers.cend(), std::back_inserter(names), [](const VkLayerProperties& p) { return p.layerName; });

    return names;
}

std::vector<const char*>
convertExtensionPropertiesToNames(const std::vector<VkExtensionProperties>& layers)
{
    std::vector<const char*> names;

    std::transform(layers.cbegin(), layers.cend(), std::back_inserter(names), [](const VkExtensionProperties& p) { return p.extensionName; });

    return names;
}

std::vector<VkLayerProperties>
getInstanceLayerProperties()
{
    // Get supported layers by vulkan
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    check(layerCount > 0);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return availableLayers;
}

std::vector<VkExtensionProperties>
getInstanceExtensionProperties()
{
    // Get supported layers by vulkan
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, nullptr);
    check(extensionCount > 0);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions.data());

    return availableExtensions;
}

std::vector<VkExtensionProperties>
getDeviceExtensionProperties(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, "", &extensionCount, nullptr);
    check(extensionCount > 0);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, "", &extensionCount, availableExtensions.data());
    return availableExtensions;
}
std::vector<VkLayerProperties>
getDeviceLayerProperties(VkPhysicalDevice device)
{
    // Get supported layers by vulkan
    uint32_t layerCount = 0;
    vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr);
    check(layerCount > 0);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateDeviceLayerProperties(device, &layerCount, availableLayers.data());
    return availableLayers;
}

VkResult
enumeratePhysicalDevices(VkInstance vkInstance, std::vector<VkPhysicalDevice>& physicalDevices)
{
    physicalDevices.clear();

    uint32_t deviceCount = 0;
    // Get number of available physical devices
    {
        const VkResult result = vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
        if (VKFAILED(result))
            {
                return result;
            }
    }
    check(deviceCount > 0);

    if (deviceCount > 0)
        {
            // Enumerate the physical devices
            physicalDevices.resize(deviceCount);

            const VkResult result = vkEnumeratePhysicalDevices(vkInstance, &deviceCount, physicalDevices.data());
            if (VKFAILED(result))
                {
                    return result;
                }
        }

    return VkResult::VK_SUCCESS;
}

uint32_t
selectPhysycalDeviceOnHighestMemory(const std::vector<VkPhysicalDevice>& physicalDevices)
{
    check(physicalDevices.size() > 0);

    // Select physical device to be used for the Vulkan example
    uint32_t selectedDevice = 0;

    // Multi-GPU - Select the device with highest memory available (usually the best gpu, but not always)
    if (physicalDevices.size() > 1)
        {
            uint64_t highestMemoryOfSelectedDevice = 0;
            for (size_t i = 0; i < physicalDevices.size(); i++)
                {
                    VkPhysicalDevice                 device       = physicalDevices[i];
                    VkPhysicalDeviceMemoryProperties deviceMemory = {};
                    vkGetPhysicalDeviceMemoryProperties(device, &deviceMemory);

                    VkPhysicalDeviceProperties prop{};
                    vkGetPhysicalDeviceProperties(device, &prop);
                    const std::string deviceName = prop.deviceName;

                    const std::string out = "Gpu index:" + std::to_string(i) + " with name:" + deviceName;

                    if (deviceMemory.memoryHeaps->size > highestMemoryOfSelectedDevice)
                        {
                            selectedDevice                = (uint32_t)i;
                            highestMemoryOfSelectedDevice = deviceMemory.memoryHeaps->size;
                        }
                }
        }
    return selectedDevice;
}

} // namespace Private

VulkanContext::VulkanContext(const DContextConfig* const config) : _warningOutput(config->warningFunction), _logOutput(config->logOutputFunction)
{
    {
        Log("Initializing VOLK");
        const VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
            {
                Log("Failed to initialize volk");
                throw std::runtime_error("Failed to initialize volk");
            }
    }

    // Initialize instance
    auto validValidationLayers = _getInstanceSupportedValidationLayers(_validationLayers);
    auto validExtensions       = _getInstanceSupportedExtensions(_instanceExtensionNames);

    if (!Instance.Init("Application", validValidationLayers, validExtensions))
        {
            throw std::runtime_error("Could not create a vulkan instance");
        }
}

bool
VulkanContext::CreateSwapchain(const WindowData* windowData, const EPresentMode& presentMode, DSwapchain* swapchain)
{
    return false;
}

void
VulkanContext::DestroySwapchain(const DSwapchain swapchain)
{
}

DFramebuffer
VulkanContext::CreateSwapchainFramebuffer()
{
    return nullptr;
}

void
VulkanContext::DestroyFramebuffer(DFramebuffer framebuffer)
{
}

void
VulkanContext::SubmitPass(RenderPassData&& data)
{
}

void
VulkanContext::SubmitCopy(CopyDataCommand&& data)
{
}

void
VulkanContext::AdvanceFrame()
{
}

unsigned char*
VulkanContext::GetAdapterDescription() const
{
    return nullptr;
}

size_t
VulkanContext::GetAdapterDedicatedVideoMemory() const
{
    return 0;
}

void
VulkanContext::OutWarning(const std::string& error)
{
    if (_warningOutput)
        {
            _warningOutput(error.data());
        }
}

void
VulkanContext::Log(const std::string& error)
{
    if (_logOutput)
        {
            _logOutput(error.data());
        }
}

std::vector<const char*>
VulkanContext::_getInstanceSupportedExtensions(const std::vector<const char*>& extentions)
{
    const auto               validExtensions      = Private::getInstanceExtensionProperties();
    const auto               validExtentionsNames = Private::convertExtensionPropertiesToNames(validExtensions);
    std::vector<const char*> supportedExtensions  = Private::filterInclusive(extentions, validExtentionsNames);
    {
        const std::string out = "Available extensions requested:" + std::to_string(supportedExtensions.size()) + "/" + std::to_string(extentions.size());
        Log(out);

        // Print not supported validation layers
        if (supportedExtensions.size() != extentions.size())
            {
                Log("Unsupported instance extensions");
                std::vector<const char*> unsupportedExtensions = Private::filterExclusive(validExtentionsNames, extentions);
                for (const char* extensionName : unsupportedExtensions)
                    {
                        Log(extensionName);
                    }
            }
    }
    return supportedExtensions;
}

std::vector<const char*>
VulkanContext::_getInstanceSupportedValidationLayers(const std::vector<const char*>& validationLayers)
{
    const std::vector<VkLayerProperties> availableLayers           = Private::getInstanceLayerProperties();
    const auto                           layerNames                = Private::convertLayerPropertiesToNames(availableLayers);
    std::vector<const char*>             supportedValidationLayers = Private::filterInclusive(validationLayers, layerNames);

    {
        const std::string out = "Available validation layers requested:" + std::to_string(supportedValidationLayers.size()) + "/" + std::to_string(validationLayers.size());
        Log(out);
        // Print not supported validation layers
        if (supportedValidationLayers.size() != validationLayers.size())
            {
                Log("Unsupported instance validation layers:");

                std::vector<const char*> unsupportedValidationLayers = Private::filterExclusive(layerNames, validationLayers);
                for (const char* layerName : unsupportedValidationLayers)
                    {
                        Log(layerName);
                    }
            }
    }

    return supportedValidationLayers;
}

VkPhysicalDevice
VulkanContext::_queryBestPhysicalDevice()
{
    std::vector<VkPhysicalDevice> physicalDevices;
    const VkResult                result = Private::enumeratePhysicalDevices(Instance.Instance, physicalDevices);
    if (VKFAILED(result))
        {
            const std::string out = "Failed to enumerate physical devices:" + std::string(VkErrorString(result));
            OutWarning(out);
            throw std::runtime_error(VkErrorString(result));
        }
    // Select gpu
    const uint32_t deviceIndex = Private::selectPhysycalDeviceOnHighestMemory(physicalDevices);
    return physicalDevices.at(deviceIndex);
}

std::vector<const char*>
VulkanContext::_getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions)
{

    const auto               validExtensions           = Private::getDeviceExtensionProperties(physicalDevice);
    const auto               extensionNames            = Private::convertExtensionPropertiesToNames(validExtensions);
    std::vector<const char*> deviceSupportedExtensions = Private::filterInclusive(extentions, extensionNames);

    const std::string out = "Available device extensions requested:" + std::to_string(deviceSupportedExtensions.size()) + "/" + std::to_string(extentions.size());
    Log(out);

    // Print not supported validation layers
    if (deviceSupportedExtensions.size() != extentions.size())
        {
            Log("Unsupported device extensions:");
            std::vector<const char*> deviceUnsupportedExtensions = Private::filterExclusive(extensionNames, extentions);
            for (const char* extensionName : deviceUnsupportedExtensions)
                {
                    Log(extensionName);
                }
        }

    return deviceSupportedExtensions;
}

std::vector<const char*>
VulkanContext::_getDeviceSupportedValidationLayers(VkPhysicalDevice physicalDevice, const std::vector<const char*>& validationLayers)
{
    const auto               validExtentions                 = Private::getDeviceExtensionProperties(physicalDevice);
    const auto               validExtentionsNames            = Private::convertExtensionPropertiesToNames(validExtentions);
    std::vector<const char*> deviceSupportedValidationLayers = Private::filterInclusive(validationLayers, validExtentionsNames);

    const std::string out = "Available device validation layers requested:" + std::to_string(deviceSupportedValidationLayers.size()) + "/" + std::to_string(validationLayers.size());
    Log(out);

    // Print not supported validation layers
    if (deviceSupportedValidationLayers.size() != validationLayers.size())
        {
            Log("Unsupported device validation layers:");
            std::vector<const char*> deviceUnsupportedValidationLayers = Private::filterExclusive(validExtentionsNames, validationLayers);
            for (const char* layerName : deviceUnsupportedValidationLayers)
                {
                    Log(layerName);
                }
        }

    return deviceSupportedValidationLayers;
}

}