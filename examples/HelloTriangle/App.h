// Copyright RedFox Studio 2022

#define GLFW_INCLUDE_NONE // makes the GLFW header not include any OpenGL or
// OpenGL ES API header.This is useful in combination with an extension loading library.
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// Include the interface and all declarations
#include "IContext.h"

#include <array>

class App
{
    static constexpr uint32_t MAX_FRAMES{ 2 };

  public:
    App();
    ~App();

    void Run();

    virtual void Draw(uint32_t cmd) = 0;

  protected:
    GLFWwindow* _window{};
    /*The rendering context*/
    Fox::IContext*    _ctx{};
    Fox::WindowData   _windowData;
    Fox::EPresentMode presentMode = Fox::EPresentMode::IMMEDIATE_KHR;
    Fox::EFormat      format      = Fox::EFormat::B8G8R8A8_UNORM;
    uint32_t          _swapchain{};
    uint32_t          _swapchainImageIndex;

    struct PerFrameData
    {
        uint32_t Fence{};
        uint32_t CmdPool{};
        uint32_t Cmd{};
        uint32_t ImageAvailableSemaphore;
        uint32_t WorkFinishedSemaphore;
        uint32_t SwapchainRenderTarget;
    };

    std::array<PerFrameData, MAX_FRAMES> _frameData;
    uint32_t                             _frameIndex{};
};