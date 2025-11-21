#include "engpch.h"
#include "render/dx12/dx12_command_queue.h"
#include "render/dx12/dx12_util.h"
#include "render/dx12/dx12_type_conversions.h"
#include "render/i_context.h"

namespace sf::render::dx12 {

DX12CommandQueue::DX12CommandQueue(ID3D12Device* device, CommandQueueType type, const wchar_t* name)
    : m_Type(type) {

    D3D12_COMMAND_QUEUE_DESC command_queue_desc{
        .Type = to_d3d12_command_list_type(type),
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0u,
    };

    dx12_check(device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&m_CommandQueue)));
    m_CommandQueue->SetName(name);

    // CPU & GPU sync fence
    dx12_check(device->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
    m_Fence->SetName(name);
}

void DX12CommandQueue::execute_command_lists(IContext** contexts, u32 count) {
    stl::vector<ID3D12CommandList*> command_lists;
    command_lists.reserve(count);

    for (u32 i = 0; i < count; ++i) {
        ID3D12GraphicsCommandList* cmd_list = static_cast<ID3D12GraphicsCommandList*>(contexts[i]->get_native_command_list());
        dx12_check(cmd_list->Close());
        command_lists.push_back(cmd_list);
    }

    m_CommandQueue->ExecuteCommandLists(static_cast<UINT>(command_lists.size()), command_lists.data());
}

void DX12CommandQueue::execute_command_list(IContext* context) {
    execute_command_lists(&context, 1);
}

u64 DX12CommandQueue::signal() {
    m_FenceValue++;
    dx12_check(m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue));
    return m_FenceValue;
}

void DX12CommandQueue::wait_for_fence_value(u64 fence_value) {
    if (!is_fence_complete(fence_value)) {
        dx12_check(m_Fence->SetEventOnCompletion(fence_value, nullptr));
    }
}

void DX12CommandQueue::wait_for_idle() {
    const u64 fence_value_to_wait_for = signal();
    wait_for_fence_value(fence_value_to_wait_for);
}

u64 DX12CommandQueue::get_last_completed_fence_value() const {
    return m_Fence->GetCompletedValue();
}

bool DX12CommandQueue::is_fence_complete(u64 fence_value) const {
    return m_Fence->GetCompletedValue() >= fence_value;
}

} // namespace sf::render::dx12
