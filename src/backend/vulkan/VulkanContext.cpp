
#include "VulkanContext.h"

#include "UtilsVK.h"

#include <string>

namespace Fox
{

VulkanContext::VulkanContext(const DContextConfig* const config) : _warningOutput(config->warningFunction), _logOutput(config->logOutputFunction)
{
    _initializeVolk();
    _initializeInstance();
    _initializeDebugger();
    _initializeVersion();
    _initializeDevice();
}

void
VulkanContext::_initializeVolk()
{
    Log("Initializing VOLK");
    const VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
        {
            Log("Failed to initialize volk");
            throw std::runtime_error("Failed to initialize volk");
        }
}

void
VulkanContext::_initializeInstance()
{
    // Initialize instance
    auto validValidationLayers = _getInstanceSupportedValidationLayers(_validationLayers);
    auto validExtensions       = _getInstanceSupportedExtensions(_instanceExtensionNames);

    if (!Instance.Init("Application", validValidationLayers, validExtensions))
        {
            throw std::runtime_error("Could not create a vulkan instance");
        }
}

void
VulkanContext::_initializeDebugger()
{
}

void
VulkanContext::_initializeVersion()
{
    auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (vkEnumerateInstanceVersion == nullptr)
        {
            OutWarning("Failed to vkGetInstanceProcAddr for vkEnumerateInstanceVersion");
        }
    else
        {
            uint32_t       instanceVersion = VK_API_VERSION_1_0;
            const VkResult result          = vkEnumerateInstanceVersion(&instanceVersion);
            if (VKFAILED(result))
                {
                    const std::string out = "Failed to vkEnumerateInstanceVersion because:" + std::string(VkUtils::VkErrorString(result));
                    OutWarning(out);
                }
            else
                {
                    // Extract version info
                    uint32_t major, minor, patch;
                    major = VK_API_VERSION_MAJOR(instanceVersion);
                    minor = VK_API_VERSION_MINOR(instanceVersion);
                    patch = VK_API_VERSION_PATCH(instanceVersion);

                    const std::string out = "Vulkan version:" + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
                    Log(out);
                }
        }
}

void
VulkanContext::_initializeDevice()
{
    // Query physical device
    VkPhysicalDevice physicalDevice = _queryBestPhysicalDevice();

    // Check validation layers and extensions support for the device
    auto validDeviceValidationLayers = _getDeviceSupportedValidationLayers(physicalDevice, _validationLayers);
    auto validDeviceExtensions       = _getDeviceSupportedExtensions(physicalDevice, _deviceExtensionNames);

    // Create device
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy                      = VK_TRUE;
    deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
    deviceFeatures.fillModeNonSolid                       = VK_TRUE;

    if (!Device.Create(Instance, physicalDevice, _deviceExtensionNames, deviceFeatures, _validationLayers))
        {
            throw std::runtime_error("Could not create a vulkan device");
        }

    // replacing global function pointers with functions retrieved with vkGetDeviceProcAddr
    volkLoadDevice(Device.Device);
}

VulkanContext::~VulkanContext()
{
    Device.Deinit();
    Instance.Deinit();
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
    const auto               validExtensions      = VkUtils::getInstanceExtensionProperties();
    const auto               validExtentionsNames = VkUtils::convertExtensionPropertiesToNames(validExtensions);
    std::vector<const char*> supportedExtensions  = VkUtils::filterInclusive(extentions, validExtentionsNames);
    {
        const std::string out = "Available extensions requested:" + std::to_string(supportedExtensions.size()) + "/" + std::to_string(extentions.size());
        Log(out);

        // Print not supported validation layers
        if (supportedExtensions.size() != extentions.size())
            {
                Log("Unsupported instance extensions");
                std::vector<const char*> unsupportedExtensions = VkUtils::filterExclusive(validExtentionsNames, extentions);
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
    const std::vector<VkLayerProperties> availableLayers           = VkUtils::getInstanceLayerProperties();
    const auto                           layerNames                = VkUtils::convertLayerPropertiesToNames(availableLayers);
    std::vector<const char*>             supportedValidationLayers = VkUtils::filterInclusive(validationLayers, layerNames);

    {
        const std::string out = "Available validation layers requested:" + std::to_string(supportedValidationLayers.size()) + "/" + std::to_string(validationLayers.size());
        Log(out);
        // Print not supported validation layers
        if (supportedValidationLayers.size() != validationLayers.size())
            {
                Log("Unsupported instance validation layers:");

                std::vector<const char*> unsupportedValidationLayers = VkUtils::filterExclusive(layerNames, validationLayers);
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
    const VkResult                result = VkUtils::enumeratePhysicalDevices(Instance.Instance, physicalDevices);
    if (VKFAILED(result))
        {
            const std::string out = "Failed to enumerate physical devices:" + std::string(VkUtils::VkErrorString(result));
            OutWarning(out);
            throw std::runtime_error(VkUtils::VkErrorString(result));
        }
    // Select gpu
    const uint32_t deviceIndex = VkUtils::selectPhysycalDeviceOnHighestMemory(physicalDevices);
    return physicalDevices.at(deviceIndex);
}

std::vector<const char*>
VulkanContext::_getDeviceSupportedExtensions(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extentions)
{

    const auto               validExtensions           = VkUtils::getDeviceExtensionProperties(physicalDevice);
    const auto               extensionNames            = VkUtils::convertExtensionPropertiesToNames(validExtensions);
    std::vector<const char*> deviceSupportedExtensions = VkUtils::filterInclusive(extentions, extensionNames);

    const std::string out = "Available device extensions requested:" + std::to_string(deviceSupportedExtensions.size()) + "/" + std::to_string(extentions.size());
    Log(out);

    // Print not supported validation layers
    if (deviceSupportedExtensions.size() != extentions.size())
        {
            Log("Unsupported device extensions:");
            std::vector<const char*> deviceUnsupportedExtensions = VkUtils::filterExclusive(extensionNames, extentions);
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
    const auto               validExtentions                 = VkUtils::getDeviceExtensionProperties(physicalDevice);
    const auto               validExtentionsNames            = VkUtils::convertExtensionPropertiesToNames(validExtentions);
    std::vector<const char*> deviceSupportedValidationLayers = VkUtils::filterInclusive(validationLayers, validExtentionsNames);

    const std::string out = "Available device validation layers requested:" + std::to_string(deviceSupportedValidationLayers.size()) + "/" + std::to_string(validationLayers.size());
    Log(out);

    // Print not supported validation layers
    if (deviceSupportedValidationLayers.size() != validationLayers.size())
        {
            Log("Unsupported device validation layers:");
            std::vector<const char*> deviceUnsupportedValidationLayers = VkUtils::filterExclusive(validExtentionsNames, validationLayers);
            for (const char* layerName : deviceUnsupportedValidationLayers)
                {
                    Log(layerName);
                }
        }

    return deviceSupportedValidationLayers;
}

}