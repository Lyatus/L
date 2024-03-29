#pragma once

#include <vulkan/vulkan.h>

#ifdef None
// Xlib may have defined some badly named macros and that breaks things
#undef None
#undef Always
#endif

#include <L/src/container/Array.h>
#include <L/src/parallelism/Lock.h>
#include <L/src/rendering/Renderer.h>
#include <L/src/rendering/Shader.h>

#define L_VK_EXT_FUNC(name,...) (PFN_##name(vkGetInstanceProcAddr(instance, #name)))(__VA_ARGS__)
#define L_VK_CHECKED(...) {VkResult result(__VA_ARGS__);if(result!=VK_SUCCESS)error("vulkan: %s:%s",#__VA_ARGS__,VulkanRenderer::result_str(result));}

struct VulkanTexture : public L::TextureImpl {
  VkFormat format;
  VkImageLayout layout;
  VkDeviceMemory memory;
  VkImage image;
  VkImageView view;
  uint32_t width, height, layer_count, mip_count;
  bool is_depth;
};

struct VulkanFramebuffer : public L::FramebufferImpl {
  L::Array<VulkanTexture*> textures;
  L::Array<VkClearValue> clear_values;
  VkFramebuffer framebuffer;
  VkRenderPass render_pass;
  uint32_t width, height;
};

struct VulkanMesh : public L::MeshImpl {
  class VulkanBuffer* vertex_buffer = nullptr;
  class VulkanBuffer* index_buffer = nullptr;
};

struct VulkanPipeline : public L::PipelineImpl {
  VkPipeline pipeline;
  VkPipelineLayout layout;
  VkDescriptorSetLayout desc_set_layout;
  L::Array<L::ShaderBinding> bindings;
  const L::RenderPassImpl* render_pass;

  const L::ShaderBinding* find_binding(const L::Symbol& name) const;
};

struct VulkanRenderPass : public L::RenderPassImpl {
  VkRenderPass render_pass;
  uint32_t color_attachment_count = 0;
};

struct VulkanShader : public L::ShaderImpl {
  VkShaderModule module;
  VkShaderStageFlags stage;
};

class VulkanRenderer : public L::Renderer {
protected:
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkPhysicalDeviceProperties physical_device_properties;
  VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
  VkPhysicalDeviceFeatures physical_device_features;
  VkDevice _device;
  VkQueue queue;
  VkSurfaceKHR surface = nullptr;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  VkSurfaceFormatKHR surface_format;
  VkSwapchainKHR swapchain;
  VkViewport _viewport = {};
  VkImage swapchain_images[2];
  VkImageView swapchain_image_views[2];
  VkFramebuffer framebuffers[2];
  VkCommandPool* command_pool;
  VkFence* command_fence;
  VkCommandBuffer render_command_buffers[2];
  VkDescriptorPool _descriptor_pool;
  VkSampler _sampler;

  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;

  L::Lock queue_lock;

  struct FlyingBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkBufferUsageFlagBits usage;
  };

  struct FlyingPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
  };

  struct FlyingSet {
    VkDescriptorSet set;
    VkPipeline pipeline;
  };

  struct GarbageImage {
    VkImage image;
    VkDeviceMemory memory;
  };

  L::Array<FlyingBuffer> used_buffers, free_buffers;
  L::Array<FlyingSet> used_sets, free_sets;
  L::Array<FlyingPipeline> garbage_pipelines;
  L::Array<VkFramebuffer> garbage_framebuffers;
  L::Array<GarbageImage> garbage_images;

#if !L_RLS
  VkDebugReportCallbackEXT debug_report_callback;
#endif

public:
  virtual bool init(L::GenericWindowData* generic_window_data) override;
  virtual void recreate_swapchain() override;
  virtual void draw(
    L::RenderCommandBuffer*,
    L::PipelineImpl*,
    L::DescriptorSetImpl*,
    const float* model,
    L::MeshImpl*,
    uint32_t vertex_count,
    uint32_t index_offset) override;

  virtual L::RenderCommandBuffer* begin_render_command_buffer() override;
  virtual void end_render_command_buffer() override;
  virtual void begin_present_pass() override;
  virtual void end_present_pass() override;

  virtual void set_scissor(L::RenderCommandBuffer*, const L::Interval2i&) override;
  virtual void reset_scissor(L::RenderCommandBuffer*) override;

  virtual void set_viewport(L::RenderCommandBuffer*, const L::Interval2i&) override;
  virtual void reset_viewport(L::RenderCommandBuffer*) override;

  virtual void begin_event(L::RenderCommandBuffer*, const char*) override;
  virtual void end_event(L::RenderCommandBuffer*) override;

  virtual L::FramebufferImpl* create_framebuffer(const L::RenderPassImpl*, const L::TextureImpl** textures, size_t texture_count) override;
  virtual void destroy_framebuffer(L::FramebufferImpl*) override;
  virtual void begin_framebuffer(L::FramebufferImpl*, L::RenderCommandBuffer*) override;
  virtual void end_framebuffer(L::FramebufferImpl*, L::RenderCommandBuffer*) override;

  virtual L::UniformBufferImpl* create_uniform_buffer(size_t size) override;
  virtual void destroy_uniform_buffer(L::UniformBufferImpl*) override;
  virtual void load_uniform_buffer(L::UniformBufferImpl*, const void* src, size_t size, size_t offset) override;

  virtual L::DescriptorSetImpl* create_descriptor_set(L::PipelineImpl*) override;
  virtual void destroy_descriptor_set(L::DescriptorSetImpl*, L::PipelineImpl*) override;
  virtual void update_descriptor_set(L::DescriptorSetImpl*, const L::ShaderBinding&, L::TextureImpl*) override;
  virtual void update_descriptor_set(L::DescriptorSetImpl*, const L::ShaderBinding&, L::UniformBufferImpl*) override;
  virtual void update_descriptor_set(L::DescriptorSetImpl*, const L::ShaderBinding&, L::FramebufferImpl*, int32_t texture_index) override;

  virtual L::ShaderImpl* create_shader(L::ShaderStage, const void* binary, size_t binary_size) override;
  virtual void destroy_shader(L::ShaderImpl*) override;

  virtual L::PipelineImpl* create_pipeline(
    const L::ShaderImpl* const* shaders,
    size_t shader_count,
    const L::ShaderBinding* bindings,
    size_t binding_count,
    const L::VertexAttribute* vertex_attributes,
    size_t vertex_attribute_count,
    const L::RenderPassImpl* render_pass,
    L::PolygonMode polygon_mode,
    L::CullMode cull_mode,
    L::PrimitiveTopology topology,
    L::BlendMode blend_mode,
    L::DepthFunc depth_func) override;
  virtual void destroy_pipeline(L::PipelineImpl*) override;

  virtual L::TextureImpl* create_texture(uint32_t width, uint32_t height, L::RenderFormat format, const void** mip_data, size_t* mip_size, size_t mip_count) override;
  virtual void load_texture(L::TextureImpl* texture, const void* data, size_t size, const L::Vector3i& offset, const L::Vector3i& extent, uint32_t mip_level) override;
  virtual void destroy_texture(L::TextureImpl* texture) override;

  virtual L::MeshImpl* create_mesh(size_t count, const void* data, size_t size, const L::VertexAttribute* attributes, size_t acount, const uint16_t* iarray, size_t icount) override;
  virtual void destroy_mesh(L::MeshImpl* mesh) override;

  virtual L::RenderPassImpl* create_render_pass(const L::RenderFormat* formats, size_t format_count, bool present, bool depth_write) override;
  virtual void destroy_render_pass(L::RenderPassImpl*) override;

  VkFormat find_supported_format(VkFormat* candidates, size_t candidate_count, VkFormatFeatureFlags features);
  uint32_t find_memory_type(uint32_t type_bits, VkMemoryPropertyFlags property_flags);

  VkCommandBuffer begin_command_buffer();
  void end_command_buffer(VkCommandBuffer);

  bool find_buffer(VkDeviceSize, VkBufferUsageFlagBits, VkBuffer&, VkDeviceMemory&);
  void destroy_buffer(VkDeviceSize, VkBufferUsageFlagBits, VkBuffer, VkDeviceMemory);
  void destroy_framebuffer(VkFramebuffer);
  void destroy_image(VkImage, VkDeviceMemory);

  VkDevice device() { return _device; }
  VkDescriptorPool descriptor_pool() { return _descriptor_pool; }
  VkSampler sampler() { return _sampler; }

  static bool is_stencil_format(VkFormat);
  static VkFormat to_vk_format(L::RenderFormat);
  static VkShaderStageFlags to_vk_shader_stage(L::ShaderStage);
  static VkPrimitiveTopology to_vk_topology(L::PrimitiveTopology);
  static VkPolygonMode to_vk_polygon_mode(L::PolygonMode);
  static VkCullModeFlagBits to_vk_cull_mode(L::CullMode);
  static VkCompareOp to_vk_depth_func(L::DepthFunc);
  static const char* result_str(VkResult result);
  static VulkanRenderer* get() { return (VulkanRenderer*)_instance; }
};
