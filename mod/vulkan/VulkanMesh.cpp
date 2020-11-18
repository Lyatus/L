#include "VulkanRenderer.h"

#include "VulkanBuffer.h"

using namespace L;

MeshImpl* VulkanRenderer::create_mesh(size_t, const void* data, size_t size, const VertexAttribute*, size_t, const uint16_t* iarray, size_t icount) {
  VulkanMesh* mesh = Memory::new_type<VulkanMesh>();

  mesh->vertex_buffer = Memory::new_type<VulkanBuffer>(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  mesh->vertex_buffer->load(data);
  if(iarray) {
    mesh->index_buffer = Memory::new_type<VulkanBuffer>(icount * sizeof(*iarray), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    mesh->index_buffer->load(iarray);
  }

  return mesh;
}

void VulkanRenderer::destroy_mesh(MeshImpl* mesh) {
  VulkanMesh* vk_mesh = (VulkanMesh*)mesh;
  if(vk_mesh->vertex_buffer) {
    Memory::delete_type(vk_mesh->vertex_buffer);
    vk_mesh->vertex_buffer = nullptr;
  }
  if(vk_mesh->index_buffer) {
    Memory::delete_type(vk_mesh->index_buffer);
    vk_mesh->index_buffer = nullptr;
  }
  Memory::delete_type(vk_mesh);
}
void VulkanRenderer::draw_mesh(RenderCommandBuffer* cmd_buffer, MeshImpl* mesh, uint32_t vertex_count, uint32_t index_offset) {
  VulkanMesh* vk_mesh = (VulkanMesh*)mesh;
  if(vk_mesh->vertex_buffer) {
    const VkBuffer buffers[] {*vk_mesh->vertex_buffer};
    const VkDeviceSize offsets[] {0};
    vkCmdBindVertexBuffers((VkCommandBuffer)cmd_buffer, 0, 1, buffers, offsets);
    if(vk_mesh->index_buffer) {
      vkCmdBindIndexBuffer((VkCommandBuffer)cmd_buffer, *vk_mesh->index_buffer, 0, VK_INDEX_TYPE_UINT16);
      vkCmdDrawIndexed((VkCommandBuffer)cmd_buffer, vertex_count, 1, index_offset, 0, 0);
    } else {
      vkCmdDraw((VkCommandBuffer)cmd_buffer, vertex_count, 1, 0, 0);
    }
  }
}
