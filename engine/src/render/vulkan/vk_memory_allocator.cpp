#include "engpch.h"

#include <vulkan/vulkan_core.h>
#include "core/logger.h"
#include "render/vulkan/vk_compat.h"
#include "render/vulkan/vk_memory_allocator.h"
#include "render/vulkan/vk_type_conversions.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace sf::render::vk {
    VkMemoryAllocator::VkMemoryAllocator(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device) : m_Device(device) {
        VmaAllocatorCreateInfo allocator_info{};
        allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
        allocator_info.instance = instance;
        allocator_info.physicalDevice = physical_device;
        allocator_info.device = device;
        vmaCreateAllocator(&allocator_info, &m_Allocator);
        CORE_INFO("Created Vulkan memory allocator (VMA)");
    }

    void VkMemoryAllocator::destroy_resources() {
        if(m_Device == VK_NULL_HANDLE)
            return;
        if (m_Allocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(m_Allocator);
            m_Allocator = VK_NULL_HANDLE;
        }
        m_Device = VK_NULL_HANDLE;
    }

    stl::result<Buffer> VkMemoryAllocator::allocate_buffer(const BufferCreationDesc& desc) {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = desc.size_in_bytes;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        Buffer buffer{};
        VkBuffer vk_buffer;
        VmaAllocation allocation;
        VK_RETURN_ON_ERROR_T(Buffer, vmaCreateBuffer(m_Allocator, &buffer_info, &alloc_info, &vk_buffer, &allocation, nullptr),
                             "Failed to allocate buffer");
        buffer.resource = reinterpret_cast<void*>(vk_buffer);
        buffer.allocation = allocation;
        buffer.size_in_bytes = desc.size_in_bytes;
        return buffer;
    }

    stl::result<Texture> VkMemoryAllocator::allocate_texture(const TextureCreationDesc& desc) {
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = desc.width;
        image_info.extent.height = desc.height;
        image_info.extent.depth = desc.depth_or_array_size;
        image_info.mipLevels = desc.mip_levels;
        image_info.arrayLayers = desc.depth_or_array_size;
        image_info.format = to_vk_format(desc.format);
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        Texture texture{};
        VkImage vk_image;
        VmaAllocation allocation;
        VK_RETURN_ON_ERROR_T(Texture, vmaCreateImage(m_Allocator, &image_info, &alloc_info, &vk_image, &allocation, nullptr),
                             "Failed to allocate texture");
        texture.resource = reinterpret_cast<void*>(vk_image);
        texture.allocation = allocation;
        texture.width = desc.width;
        texture.height = desc.height;
        texture.depth_or_array_size = desc.depth_or_array_size;
        texture.format = desc.format;
        return texture;
    }

    void VkMemoryAllocator::free_buffer(Buffer& buffer) {
        if (buffer.resource && buffer.allocation) {
            vmaDestroyBuffer(m_Allocator, reinterpret_cast<VkBuffer>(buffer.resource), static_cast<VmaAllocation>(buffer.allocation));
            buffer.resource = nullptr;
            buffer.allocation = nullptr;
        }
    }

    void VkMemoryAllocator::free_texture(Texture& texture) {
        if (texture.resource && texture.allocation) {
            vmaDestroyImage(m_Allocator, reinterpret_cast<VkImage>(texture.resource), static_cast<VmaAllocation>(texture.allocation));
            texture.resource = nullptr;
            texture.allocation = nullptr;
        }
    }

    void VkMemoryAllocator::get_stats(void* stats_out) {
        if (stats_out) {
            vmaCalculateStatistics(m_Allocator, static_cast<VmaTotalStatistics*>(stats_out));
        }
    }
} // namespace sf::render::vk
