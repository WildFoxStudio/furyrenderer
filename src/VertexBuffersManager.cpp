#include "VertexBuffersManager.h"

#include "Core/Engine.h"
#include "RenderInterface/NullThread.h"

namespace Fox
{
void
CVertexBufferManager::Initialize()
{
    _factory = (RIFactoryVk*)CNullThread::GetInstance()->GetContext()->QueryFactoryInterface();
    check(_factory);
}

void
CVertexBufferManager::Deinitialize()
{
}

std::shared_ptr<DRIBuffer>
CVertexBufferManager::CreateVertexBuffer(const DGUID& guid, void* data, size_t length)
{
    DRIBuffer buf;
    buf.Buffer = _factory->MT_CreateMappedBufferGpuOnly(length, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, buf.Allocation);

    auto bufferPtr = std::make_shared<DRIBuffer>(buf);

    _guidToVertexBuffer[guid] = bufferPtr;

    return bufferPtr;
}

}