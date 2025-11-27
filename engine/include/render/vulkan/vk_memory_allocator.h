#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include "render/i_memory_allocator.h"

namespace sf::render::vk {

    class VkMemoryAllocator : public IMemoryAllocator {
    public:
        explicit VkMemoryAllocator(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device);
        VkMemoryAllocator() = default;
        ~VkMemoryAllocator() override;

        void destroy_resources();

        stl::result<Buffer> allocate_buffer(const BufferCreationDesc& desc) override;
        stl::result<Texture> allocate_texture(const TextureCreationDesc& desc) override;

        void free_buffer(Buffer& buffer) override;
        void free_texture(Texture& texture) override;

        void get_stats(void* stats_out) override;
        void* get_native_allocator() override { return m_Allocator; }

        VmaAllocator get_vma_allocator() const { return m_Allocator; }

    private:
        VkDevice m_Device {VK_NULL_HANDLE};
        VmaAllocator m_Allocator {VK_NULL_HANDLE};
    };

} // namespace sf::render::vk
