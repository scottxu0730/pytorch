#include <ATen/native/vulkan/api/Descriptor.h>

namespace at {
namespace native {
namespace vulkan {
namespace api {
namespace {

VkDescriptorPool create_descriptor_pool(
    const VkDevice device) {
  const struct {
    uint32_t capacity;
    c10::SmallVector<VkDescriptorPoolSize, 16u> sizes;
  } descriptor {
    1024u,
    {
      // Note: It is OK for the sum of descriptors per type, below, to exceed
      // the max total figure above, but be concenious of memory consumption.
      // Considering how the descriptor pool must be frequently purged anyway
      // as a result of the impracticality of having enormous pools that
      // persist through the execution of the program, there is diminishing
      // return in increasing max counts.
      {
        /*
          Buffers
        */

        {
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          256u,
        },
        {
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          256u,
        },

        /*
          Images
        */

        {
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          256u,
        },
        {
          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          256u,
        },
      },
    },
  };

  const VkDescriptorPoolCreateInfo descriptor_pool_create_info{
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    nullptr,
    0u, /* Do not use VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT */
    descriptor.capacity,
    static_cast<uint32_t>(descriptor.sizes.size()),
    descriptor.sizes.data(),
  };

  VkDescriptorPool descriptor_pool{};
  VK_CHECK(vkCreateDescriptorPool(
      device,
      &descriptor_pool_create_info,
      nullptr,
      &descriptor_pool));

  TORCH_CHECK(
      descriptor_pool,
      "Invalid Vulkan descriptor pool!");

  return descriptor_pool;
}

VkDescriptorSet allocate_descriptor_set(
    const VkDevice device,
    const VkDescriptorPool descriptor_pool,
    const VkDescriptorSetLayout descriptor_set_layout) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(
      device,
      "Invalid Vulkan device!");

  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(
      descriptor_pool,
      "Invalid Vulkan descriptor pool!");

  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(
      descriptor_set_layout,
      "Invalid Vulkan descriptor set layout!");

  const VkDescriptorSetAllocateInfo descriptor_set_allocate_info{
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    nullptr,
    descriptor_pool,
    1u,
    &descriptor_set_layout,
  };

  VkDescriptorSet descriptor_set{};
  VK_CHECK(vkAllocateDescriptorSets(
      device,
      &descriptor_set_allocate_info,
      &descriptor_set));

  TORCH_CHECK(
      descriptor_set,
      "Invalid Vulkan descriptor set!");

  return descriptor_set;
}

} // namespace

Descriptor::Set::Set(
    const VkDevice device,
    const VkDescriptorPool descriptor_pool,
    const VkDescriptorSetLayout descriptor_set_layout)
  : device_(device),
    descriptor_set_(
        allocate_descriptor_set(
            device_,
            descriptor_pool,
            descriptor_set_layout)) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(
      descriptor_set_,
      "Invalid Vulkan descriptor set!");
}

Descriptor::Pool::Pool(const GPU& gpu)
  : device_(gpu.device),
    descriptor_pool_(
        create_descriptor_pool(gpu.device),
        VK_DELETER(DescriptorPool)(device_)) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(
      device_,
      "Invalid Vulkan device!");

  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(
      descriptor_pool_,
      "Invalid Vulkan descriptor pool!");
}

Descriptor::Set Descriptor::Pool::allocate(
    const VkDescriptorSetLayout descriptor_set_layout)
{
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(
      descriptor_set_layout,
      "Invalid Vulkan descriptor set layout!");

  return Set(
      device_,
      descriptor_pool_.get(),
      descriptor_set_layout);
}

void Descriptor::Pool::purge() {
  VK_CHECK(vkResetDescriptorPool(device_, descriptor_pool_.get(), 0u));
}

} // namespace api
} // namespace vulkan
} // namespace native
} // namespace at
