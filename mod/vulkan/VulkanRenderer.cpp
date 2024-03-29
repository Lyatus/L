#if L_USE_MODULE_win32
#define VK_USE_PLATFORM_WIN32_KHR
#include "win32.h"
#endif
#if L_USE_MODULE_xlib
#define VK_USE_PLATFORM_XLIB_KHR
#include "xlib.h"
#endif

#include <L/src/dev/debug.h>
#include <L/src/dev/profiling.h>
#include <L/src/macros.h>
#include <L/src/system/Window.h>
#include <L/src/text/format.h>

#include "VulkanBuffer.h"
#include "VulkanRenderer.h"

using namespace L;

#if !L_RLS
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char* msg, void*) {
  warning("Vulkan: %s", msg);
  return VK_FALSE;
}
#endif

bool VulkanRenderer::init(GenericWindowData* generic_window_data) {
  L_SCOPE_MARKER("VulkanRenderer::init");

  { // Create instance
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pEngineName = "L Engine";
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    const char* window_manager_ext = nullptr;
#if L_USE_MODULE_win32
    if(!window_manager_ext && generic_window_data->type == win32_window_type) {
      window_manager_ext = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    }
#endif
#if L_USE_MODULE_xlib
    if(!window_manager_ext && generic_window_data->type == xlib_window_type) {
      window_manager_ext = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
    }
#endif

    if(window_manager_ext == nullptr) {
      warning("vulkan: Unknown window manager %s", (const char*)generic_window_data->type);
      return false;
    }

    const char* required_extensions[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      window_manager_ext,
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#if !L_RLS
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
    };
    create_info.enabledExtensionCount = L_COUNT_OF(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;

#if !L_RLS
    { // Layers
      // Enumerate all available layers
      VkLayerProperties layer_properties[32];
      uint32_t layer_property_count = L_COUNT_OF(layer_properties);
      vkEnumerateInstanceLayerProperties(&layer_property_count, layer_properties);

      // List wanted layers
      const char* wanted_layers[] = {
        "VK_LAYER_LUNARG_standard_validation",
        "VK_LAYER_KHRONOS_validation",
      };
      uint32_t wanted_layer_count = L_COUNT_OF(wanted_layers);

      // Remove wanted layers that are not available
      for(uintptr_t i = 0; i < wanted_layer_count; i++) {
        bool found_layer = false;
        for(uintptr_t j = 0; j < layer_property_count; j++) {
          if(!strcmp(layer_properties[j].layerName, wanted_layers[i])) {
            found_layer = true;
            break;
          }
        }
        if(!found_layer) {
          wanted_layers[i--] = wanted_layers[--wanted_layer_count];
        }
      }

      create_info.enabledLayerCount = wanted_layer_count;
      create_info.ppEnabledLayerNames = wanted_layers;
    }
#endif

    L_VK_CHECKED(vkCreateInstance(&create_info, nullptr, &instance));
  }

#if !L_RLS
  { // Create debug report callback
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debug_callback;

    L_VK_CHECKED(L_VK_EXT_FUNC(vkCreateDebugReportCallbackEXT, instance, &createInfo, nullptr, &debug_report_callback));
  }
#endif

  { // Select physical device
    VkPhysicalDevice physical_devices[16];
    uint32_t physical_device_count = L_COUNT_OF(physical_devices);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);
    if(physical_device_count == 0) {
      warning("vulkan: No physical device");
      return false;
    }

    uintptr_t best_physical_device(0);
    size_t best_physical_device_memory(0);

    { // Choose best physical device based on dedicated memory
      for(uintptr_t i(0); i < physical_device_count; i++) {
        size_t physical_device_memory(0);
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &physical_device_memory_properties);
        for(uintptr_t j(0); j < physical_device_memory_properties.memoryHeapCount; j++) {
          if(physical_device_memory_properties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            physical_device_memory += physical_device_memory_properties.memoryHeaps[j].size;
          }
        }
        if(physical_device_memory > best_physical_device_memory) {
          best_physical_device_memory = physical_device_memory;
          best_physical_device = i;
        }
      }
      physical_device = physical_devices[best_physical_device];
    }

    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
    log("Vulkan version: %d.%d.%d",
      VK_VERSION_MAJOR(physical_device_properties.apiVersion),
      VK_VERSION_MINOR(physical_device_properties.apiVersion),
      VK_VERSION_PATCH(physical_device_properties.apiVersion));
    log("GPU: %s", physical_device_properties.deviceName);
    log("GPU memory: %s", format_memory_amount(best_physical_device_memory).begin());
  }

  { // Create surface
#if L_USE_MODULE_win32
    if(!surface && generic_window_data->type == win32_window_type) {
      Win32WindowData* win32_window_data = (Win32WindowData*)generic_window_data;
      VkWin32SurfaceCreateInfoKHR create_info = {};
      create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
      create_info.hinstance = win32_window_data->module;
      create_info.hwnd = win32_window_data->window;
      L_VK_CHECKED(vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface));
    }
#endif
#if L_USE_MODULE_xlib
    if(!surface && generic_window_data->type == xlib_window_type) {
      XlibWindowData* win_data = (XlibWindowData*)generic_window_data;
      VkXlibSurfaceCreateInfoKHR create_info {};
      create_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
      create_info.dpy = win_data->display;
      create_info.window = win_data->window;
      L_VK_CHECKED(vkCreateXlibSurfaceKHR(instance, &create_info, nullptr, &surface));
    }
#endif
    L_ASSERT(surface);
  }

  uint32_t queue_family_index(UINT32_MAX);
  { // Fetch queue family index
    uint32_t count(0);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    VkQueueFamilyProperties queue_family_properties[16];
    L_ASSERT(count < L_COUNT_OF(queue_family_properties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queue_family_properties);
    for(uint32_t i(0); i < count; i++) {
      VkBool32 present_support;
      vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
      if(present_support && queue_family_properties[i].queueCount > 0 && queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        queue_family_index = i;
    }
    L_ASSERT(queue_family_index != UINT32_MAX);
  }

  { // Create device and fetch queue
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family_index;
    queue_create_info.queueCount = 1;
    float priority(1.f);
    queue_create_info.pQueuePriorities = &priority;

    const char* required_extensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceFeatures enabled_features {};
    enabled_features.fillModeNonSolid = true;
    enabled_features.samplerAnisotropy = true;
    enabled_features.textureCompressionBC = true;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.queueCreateInfoCount = 1;
    create_info.enabledExtensionCount = L_COUNT_OF(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;
    create_info.pEnabledFeatures = &enabled_features;

    L_VK_CHECKED(vkCreateDevice(physical_device, &create_info, nullptr, &_device));

    vkGetDeviceQueue(_device, queue_family_index, 0, &queue);
  }

  { // Select surface format
    VkSurfaceFormatKHR surface_formats[16];
    uint32_t surface_format_count(0);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
    L_ASSERT(surface_format_count < L_COUNT_OF(surface_formats));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats);
    for(uint32_t i(0); i < surface_format_count; i++)
      if(surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        surface_format = surface_formats[i];
  }

  { // Create command pools
    const uint32_t command_pool_count(TaskSystem::fiber_count());
    command_pool = Memory::alloc_type<VkCommandPool>(command_pool_count);

    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.queueFamilyIndex = queue_family_index;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    for(uint32_t i(0); i < command_pool_count; i++) {
      L_VK_CHECKED(vkCreateCommandPool(_device, &create_info, nullptr, command_pool + i));
    }
  }

  { // Create command fences
    const uint32_t command_fence_count(TaskSystem::fiber_count());
    command_fence = Memory::alloc_type<VkFence>(command_fence_count);

    VkFenceCreateInfo create_info {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    for(uint32_t i(0); i < command_fence_count; i++) {
      L_VK_CHECKED(vkCreateFence(_device, &create_info, nullptr, command_fence + i));
    }
  }

  { // Create command buffers
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = *command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)L_COUNT_OF(render_command_buffers);

    L_VK_CHECKED(vkAllocateCommandBuffers(_device, &allocInfo, render_command_buffers));
  }

  { // Create semaphores
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    L_VK_CHECKED(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
    L_VK_CHECKED(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
  }

  { // Create descriptor pool
    VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 << 12},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 << 10},
    };

    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = L_COUNT_OF(pool_sizes);
    create_info.pPoolSizes = pool_sizes;
    create_info.maxSets = 1 << 16;
    create_info.flags = VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    L_VK_CHECKED(vkCreateDescriptorPool(_device, &create_info, nullptr, &_descriptor_pool));
  }

  { // Create default sampler
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 256.0f;

    L_VK_CHECKED(vkCreateSampler(VulkanRenderer::device(), &samplerInfo, nullptr, &_sampler));
  }

  recreate_swapchain();

  return true;
}
void VulkanRenderer::recreate_swapchain() {
  vkDeviceWaitIdle(VulkanRenderer::device());

  { // Cleanup
    for(uintptr_t i(0); i < L_COUNT_OF(framebuffers); i++) {
      vkDestroyFramebuffer(VulkanRenderer::device(), framebuffers[i], nullptr);
    }

    for(uintptr_t i(0); i < L_COUNT_OF(swapchain_image_views); i++) {
      vkDestroyImageView(VulkanRenderer::device(), swapchain_image_views[i], nullptr);
    }

    vkDestroySwapchainKHR(VulkanRenderer::device(), swapchain, nullptr);
  }

  { // Fetch surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
    L_ASSERT(surface_capabilities.currentExtent.width != UINT32_MAX);
  }

  { // Setup _viewport
    _viewport.x = 0.0f;
    _viewport.y = 0.0f;
    _viewport.width = (float)surface_capabilities.currentExtent.width;
    _viewport.height = (float)surface_capabilities.currentExtent.height;
    _viewport.minDepth = 0.0f;
    _viewport.maxDepth = 1.0f;
  }

  { // Create swapchain
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = 2;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = surface_capabilities.currentExtent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    L_VK_CHECKED(vkCreateSwapchainKHR(_device, &create_info, nullptr, &swapchain));
  }

  { // Fetch swapchain images
    uint32_t swapchain_image_count;
    vkGetSwapchainImagesKHR(_device, swapchain, &swapchain_image_count, nullptr);
    L_ASSERT(swapchain_image_count == 2);
    vkGetSwapchainImagesKHR(_device, swapchain, &swapchain_image_count, swapchain_images);
  }

  { // Create swapchain image views
    for(uint32_t i(0); i < 2; i++) {
      VkImageViewCreateInfo create_info {};
      create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image = swapchain_images[i];
      create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format = surface_format.format;
      create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      create_info.subresourceRange.baseMipLevel = 0;
      create_info.subresourceRange.levelCount = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount = 1;

      L_VK_CHECKED(vkCreateImageView(_device, &create_info, nullptr, swapchain_image_views + i));
    }
  }

  init_render_passes();

  { // Create framebuffers
    for(size_t i = 0; i < L_COUNT_OF(swapchain_image_views); i++) {
      VkImageView attachments[] = {
        swapchain_image_views[i],
      };

      VkFramebufferCreateInfo framebufferInfo = {};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = ((VulkanRenderPass*)_present_pass)->render_pass;
      framebufferInfo.attachmentCount = L_COUNT_OF(attachments);
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = surface_capabilities.currentExtent.width;
      framebufferInfo.height = surface_capabilities.currentExtent.height;
      framebufferInfo.layers = 1;

      L_VK_CHECKED(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &framebuffers[i]));
    }
  }
}
void VulkanRenderer::draw(
  L::RenderCommandBuffer* cmd_buffer,
  L::PipelineImpl* pipeline,
  L::DescriptorSetImpl* desc_set,
  const float* model,
  L::MeshImpl* mesh,
  uint32_t vertex_count,
  uint32_t index_offset) {
  VulkanPipeline* vk_pipeline = (VulkanPipeline*)pipeline;
  VulkanMesh* vk_mesh = (VulkanMesh*)mesh;

  vkCmdBindDescriptorSets((VkCommandBuffer)cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->layout, 0, 1, (VkDescriptorSet*)&desc_set, 0, nullptr);
  if(vk_pipeline->find_binding("Constants") != nullptr) {
    vkCmdPushConstants((VkCommandBuffer)cmd_buffer, vk_pipeline->layout, VK_SHADER_STAGE_ALL, 0, sizeof(Matrix44f), model);
  }
  vkCmdBindPipeline((VkCommandBuffer)cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->pipeline);

  if(vk_mesh && vk_mesh->vertex_buffer) {
    const VkBuffer buffers[] {*vk_mesh->vertex_buffer};
    const VkDeviceSize offsets[] {0};
    vkCmdBindVertexBuffers((VkCommandBuffer)cmd_buffer, 0, 1, buffers, offsets);
    if(vk_mesh->index_buffer) {
      vkCmdBindIndexBuffer((VkCommandBuffer)cmd_buffer, *vk_mesh->index_buffer, 0, VK_INDEX_TYPE_UINT16);
      vkCmdDrawIndexed((VkCommandBuffer)cmd_buffer, vertex_count, 1, index_offset, 0, 0);
    } else {
      vkCmdDraw((VkCommandBuffer)cmd_buffer, vertex_count, 1, 0, 0);
    }
  } else {
    vkCmdDraw((VkCommandBuffer)cmd_buffer, vertex_count, 1, 0, 0);
  }
}

static uint32_t image_index;
RenderCommandBuffer* VulkanRenderer::begin_render_command_buffer() {
  vkAcquireNextImageKHR(_device, swapchain, uint64_t(-1), imageAvailableSemaphore, VK_NULL_HANDLE, &image_index);
  VkCommandBuffer cmd_buffer(render_command_buffers[image_index]);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr; // Optional

  L_VK_CHECKED(vkBeginCommandBuffer(cmd_buffer, &beginInfo));
  return (RenderCommandBuffer*)cmd_buffer;
}
void VulkanRenderer::end_render_command_buffer() {
  VkCommandBuffer cmd_buffer(render_command_buffers[image_index]);

  L_VK_CHECKED(vkEndCommandBuffer(cmd_buffer));

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd_buffer;

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &image_index;

  {
    L_SCOPE_MARKER("Present");
    L_SCOPED_LOCK(queue_lock);
    L_VK_CHECKED(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    if(VkResult result = vkQueuePresentKHR(queue, &presentInfo)) {
      if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
      } else {
        error(VulkanRenderer::result_str(result));
      }
    }
    vkQueueWaitIdle(queue);
  }

  for(const FlyingBuffer& buffer : used_buffers) {
    free_buffers.push(buffer);
  }
  used_buffers.clear();
  for(const FlyingSet& set : used_sets) {
    free_sets.push(set);
  }
  used_sets.clear();
  for(const VkFramebuffer& framebuffer : garbage_framebuffers) {
    vkDestroyFramebuffer(VulkanRenderer::device(), framebuffer, nullptr);
  }
  garbage_framebuffers.clear();
  for(const GarbageImage& image : garbage_images) {
    vkDestroyImage(VulkanRenderer::device(), image.image, nullptr);
    vkFreeMemory(VulkanRenderer::device(), image.memory, nullptr);
  }
  garbage_images.clear();
  for(const FlyingPipeline& pipeline : garbage_pipelines) {
    vkDestroyPipeline(_device, pipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(_device, pipeline.layout, nullptr);

    // Destroy all related descriptor sets
    for(uintptr_t i = 0; i < free_sets.size(); i++) {
      const FlyingSet& flying_set = free_sets[i];
      if(flying_set.pipeline == pipeline.pipeline) {
        vkFreeDescriptorSets(_device, _descriptor_pool, 1, &flying_set.set);
        free_sets.erase_fast(i);
      }
    }
  }
  garbage_pipelines.clear();
}
void VulkanRenderer::begin_present_pass() {
  VkCommandBuffer cmd_buffer(render_command_buffers[image_index]);
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = ((VulkanRenderPass*)_present_pass)->render_pass;
  renderPassInfo.framebuffer = framebuffers[image_index];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = VkExtent2D {uint32_t(_viewport.width), uint32_t(_viewport.height)};
  VkClearValue clear_value {0.f, 0.f, 0.f, 1.f};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clear_value;

  vkCmdBeginRenderPass(cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}
void VulkanRenderer::end_present_pass() {
  VkCommandBuffer cmd_buffer(render_command_buffers[image_index]);
  vkCmdEndRenderPass(cmd_buffer);
}

void VulkanRenderer::set_scissor(RenderCommandBuffer* cmd_buffer, const L::Interval2i& scissor) {
  const Vector2f scissor_size = scissor.size();
  VkRect2D vk_scissor = {
    VkOffset2D {scissor.min().x(), scissor.min().y()},
    VkExtent2D {uint32_t(scissor_size.x()), uint32_t(scissor_size.y())},
  };
  vkCmdSetScissor((VkCommandBuffer)cmd_buffer, 0, 1, &vk_scissor);
}
void VulkanRenderer::reset_scissor(RenderCommandBuffer* cmd_buffer) {
  static const VkRect2D vk_scissor = {
    VkOffset2D {0, 0},
    VkExtent2D {1 << 14, 1 << 14},
  };
  vkCmdSetScissor((VkCommandBuffer)cmd_buffer, 0, 1, &vk_scissor);
}

void VulkanRenderer::set_viewport(RenderCommandBuffer* cmd_buffer, const Interval2i& viewport) {
  const Vector2i viewport_size = viewport.size();
  const VkViewport vk_viewport = {
    float(viewport.min().x()),
    float(viewport.min().y()),
    float(viewport_size.x()),
    float(viewport_size.y()),
    0.f,
    1.f,
  };
  vkCmdSetViewport((VkCommandBuffer)cmd_buffer, 0, 1, &vk_viewport);
}
void VulkanRenderer::reset_viewport(RenderCommandBuffer*) {
#if 0
  VkViewport viewport {0,0,float(_geometry_buffer.width()),float(_geometry_buffer.height()),0.f,1.f};
  vkCmdSetViewport((VkCommandBuffer)cmd_buffer, 0, 1, &viewport);
#endif
}

void VulkanRenderer::begin_event(RenderCommandBuffer* cmd_buffer, const char* name) {
  VkDebugUtilsLabelEXT marker_info = {};
  marker_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  marker_info.pLabelName = name;
  L_VK_EXT_FUNC(vkCmdBeginDebugUtilsLabelEXT, (VkCommandBuffer)cmd_buffer, &marker_info);
}
void VulkanRenderer::end_event(RenderCommandBuffer* cmd_buffer) {
  L_VK_EXT_FUNC(vkCmdEndDebugUtilsLabelEXT, (VkCommandBuffer)cmd_buffer);
}

VkFormat VulkanRenderer::find_supported_format(VkFormat* candidates, size_t candidate_count, VkFormatFeatureFlags features) {
  for(uintptr_t i(0); i < candidate_count; i++) {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &properties);
    if((properties.optimalTilingFeatures & features) == features)
      return candidates[i];
  }
  return VK_FORMAT_UNDEFINED;
}
uint32_t VulkanRenderer::find_memory_type(uint32_t type_bits, VkMemoryPropertyFlags property_flags) {
  for(uint32_t i(0); i < physical_device_memory_properties.memoryTypeCount; i++)
    if((type_bits & (1 << i)) && (physical_device_memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags)
      return i;
  error("vulkan: failed to find suitable memory");
  return 0;
}

VkCommandBuffer VulkanRenderer::begin_command_buffer() {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = command_pool[TaskSystem::fiber_id()];
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void VulkanRenderer::end_command_buffer(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  {
    L_SCOPE_MARKER("Command buffer fence");
    L_SCOPED_LOCK(queue_lock);
    VkFence fence(command_fence[TaskSystem::fiber_id()]);
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    TaskSystem::yield_until([](void* fence) {
      return vkGetFenceStatus(VulkanRenderer::get()->device(), *(VkFence*)fence) == VK_SUCCESS;
      }, &fence);
    vkResetFences(_device, 1, &fence);
  }

  vkFreeCommandBuffers(_device, command_pool[TaskSystem::fiber_id()], 1, &commandBuffer);
}

bool VulkanRenderer::find_buffer(VkDeviceSize size, VkBufferUsageFlagBits usage, VkBuffer& buffer, VkDeviceMemory& memory) {
  for(uintptr_t i(0); i < free_buffers.size(); i++) {
    const FlyingBuffer& flying_buffer(free_buffers[i]);
    if(flying_buffer.size == size && flying_buffer.usage == usage) {
      buffer = flying_buffer.buffer;
      memory = flying_buffer.memory;
      free_buffers.erase_fast(i);
      return true;
    }
  }
  return false;
}
void VulkanRenderer::destroy_buffer(VkDeviceSize size, VkBufferUsageFlagBits usage, VkBuffer buffer, VkDeviceMemory memory) {
  used_buffers.push(FlyingBuffer {buffer, memory, size, usage});
}
void VulkanRenderer::destroy_framebuffer(VkFramebuffer framebuffer) {
  garbage_framebuffers.push(framebuffer);
}
void VulkanRenderer::destroy_image(VkImage image, VkDeviceMemory memory) {
  garbage_images.push(GarbageImage {image, memory});
}

bool VulkanRenderer::is_stencil_format(VkFormat format) {
  switch(format) {
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return true;
    default: return false;
  }
}
VkFormat VulkanRenderer::to_vk_format(L::RenderFormat format) {
  switch(format) {
    case RenderFormat::R8_UNorm: return VK_FORMAT_R8_UNORM;
    case RenderFormat::R8G8B8A8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
    case RenderFormat::B8G8R8A8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;

    case RenderFormat::D16_UNorm: return VK_FORMAT_D16_UNORM;
    case RenderFormat::D16_UNorm_S8_UInt: return VK_FORMAT_D16_UNORM_S8_UINT;
    case RenderFormat::R16_SFloat: return VK_FORMAT_R16_SFLOAT;
    case RenderFormat::R16_SInt: return VK_FORMAT_R16_SINT;
    case RenderFormat::R16_UInt: return VK_FORMAT_R16_UINT;
    case RenderFormat::R16G16_SFloat: return VK_FORMAT_R16G16_SFLOAT;
    case RenderFormat::R16G16_SInt: return VK_FORMAT_R16G16_SINT;
    case RenderFormat::R16G16_UInt: return VK_FORMAT_R16G16_UINT;
    case RenderFormat::R16G16B16_SFloat: return VK_FORMAT_R16G16B16_SFLOAT;
    case RenderFormat::R16G16B16_SInt: return VK_FORMAT_R16G16B16_SINT;
    case RenderFormat::R16G16B16_UInt: return VK_FORMAT_R16G16B16_UINT;
    case RenderFormat::R16G16B16A16_SFloat: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case RenderFormat::R16G16B16A16_SInt: return VK_FORMAT_R16G16B16A16_SINT;
    case RenderFormat::R16G16B16A16_UInt: return VK_FORMAT_R16G16B16A16_UINT;
    case RenderFormat::R16G16B16A16_UNorm: return VK_FORMAT_R16G16B16A16_UNORM;

    case RenderFormat::D24_UNorm_S8_UInt: return VK_FORMAT_D24_UNORM_S8_UINT;

    case RenderFormat::D32_SFloat: return VK_FORMAT_D32_SFLOAT;
    case RenderFormat::D32_SFloat_S8_UInt: return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case RenderFormat::R32_SFloat: return VK_FORMAT_R32_SFLOAT;
    case RenderFormat::R32_SInt: return VK_FORMAT_R32_SINT;
    case RenderFormat::R32_UInt: return VK_FORMAT_R32_UINT;
    case RenderFormat::R32G32_SFloat: return VK_FORMAT_R32G32_SFLOAT;
    case RenderFormat::R32G32_SInt: return VK_FORMAT_R32G32_SINT;
    case RenderFormat::R32G32_UInt: return VK_FORMAT_R32G32_UINT;
    case RenderFormat::R32G32B32_SFloat: return VK_FORMAT_R32G32B32_SFLOAT;
    case RenderFormat::R32G32B32_SInt: return VK_FORMAT_R32G32B32_SINT;
    case RenderFormat::R32G32B32_UInt: return VK_FORMAT_R32G32B32_UINT;
    case RenderFormat::R32G32B32A32_SFloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case RenderFormat::R32G32B32A32_SInt: return VK_FORMAT_R32G32B32A32_SINT;
    case RenderFormat::R32G32B32A32_UInt: return VK_FORMAT_R32G32B32A32_UINT;

    case RenderFormat::BC1_RGB_UNorm_Block: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case RenderFormat::BC3_UNorm_Block: return VK_FORMAT_BC3_UNORM_BLOCK;
    case RenderFormat::BC4_UNorm_Block: return VK_FORMAT_BC4_UNORM_BLOCK;
    default: L::error("vulkan: Unknown render format");
  }

  return VK_FORMAT_UNDEFINED;
}
VkShaderStageFlags VulkanRenderer::to_vk_shader_stage(L::ShaderStage stage) {
  switch(stage) {
    case L::ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
    case L::ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
    case L::ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
    default: return VK_SHADER_STAGE_ALL;
  }
}
VkPrimitiveTopology VulkanRenderer::to_vk_topology(L::PrimitiveTopology topology) {
  switch(topology) {
    case PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case PrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case PrimitiveTopology::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    default: return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
  }
}
VkPolygonMode VulkanRenderer::to_vk_polygon_mode(L::PolygonMode polygon_mode) {
  switch(polygon_mode) {
    case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
    case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
    case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
    default: return VK_POLYGON_MODE_MAX_ENUM;
  }
}
VkCullModeFlagBits VulkanRenderer::to_vk_cull_mode(L::CullMode cull_mode) {
  switch(cull_mode) {
    case L::CullMode::None: return VK_CULL_MODE_NONE;
    case L::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
    case L::CullMode::Back: return VK_CULL_MODE_BACK_BIT;
    case L::CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
    default: return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
  }
}
VkCompareOp VulkanRenderer::to_vk_depth_func(L::DepthFunc depth_func) {
  switch(depth_func) {
    case L::DepthFunc::Undefined: return VK_COMPARE_OP_MAX_ENUM;
    case L::DepthFunc::Never: return VK_COMPARE_OP_NEVER;
    case L::DepthFunc::Always: return VK_COMPARE_OP_ALWAYS;
    case L::DepthFunc::Less: return VK_COMPARE_OP_LESS;
    case L::DepthFunc::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
    case L::DepthFunc::Greater: return VK_COMPARE_OP_GREATER;
    case L::DepthFunc::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case L::DepthFunc::Equal: return VK_COMPARE_OP_EQUAL;
    case L::DepthFunc::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
  }
  return VK_COMPARE_OP_MAX_ENUM;
}

const char* VulkanRenderer::result_str(VkResult result) {
#define CASE(v) case v: return #v
  switch(result) {
    CASE(VK_SUCCESS);
    CASE(VK_NOT_READY);
    CASE(VK_TIMEOUT);
    CASE(VK_EVENT_SET);
    CASE(VK_EVENT_RESET);
    CASE(VK_INCOMPLETE);
    CASE(VK_ERROR_OUT_OF_HOST_MEMORY);
    CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    CASE(VK_ERROR_INITIALIZATION_FAILED);
    CASE(VK_ERROR_DEVICE_LOST);
    CASE(VK_ERROR_MEMORY_MAP_FAILED);
    CASE(VK_ERROR_LAYER_NOT_PRESENT);
    CASE(VK_ERROR_EXTENSION_NOT_PRESENT);
    CASE(VK_ERROR_FEATURE_NOT_PRESENT);
    CASE(VK_ERROR_INCOMPATIBLE_DRIVER);
    CASE(VK_ERROR_TOO_MANY_OBJECTS);
    CASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
    default: return "unknown";
  }
#undef CASE
}