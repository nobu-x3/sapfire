#pragma once
#ifdef SAPFIRE_PLATFORM_WINDOWS
#include "render/i_command_queue.h"
#include <wrl/client.h>
#include <d3d12.h>

namespace sf::render::dx12 {

class DX12CommandQueue : public ICommandQueue {
public:
    explicit DX12CommandQueue(ID3D12Device* device, CommandQueueType type, const wchar_t* name);
    ~DX12CommandQueue() override = default;

    // ICommandQueue implementation
    void execute_command_lists(IContext** contexts, u32 count) override;
    void execute_command_list(IContext* context) override;

    u64 signal() override;
    void wait_for_fence_value(u64 fence_value) override;
    void wait_for_idle() override;
    u64 get_last_completed_fence_value() const override;

    CommandQueueType get_type() const override { return m_Type; }
    void* get_native_handle() override { return m_CommandQueue.Get(); }

    // DX12-specific
    ID3D12CommandQueue* get_d3d12_queue() const { return m_CommandQueue.Get(); }
    ID3D12Fence* get_d3d12_fence() const { return m_Fence.Get(); }
    bool is_fence_complete(u64 fence_value) const;

private:
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
    u64 m_FenceValue = 0;
    CommandQueueType m_Type;
};

} // namespace sf::render::dx12
#endif
