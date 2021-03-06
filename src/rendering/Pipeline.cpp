#include "Pipeline.h"

#include "../engine/Engine.h"
#include "../engine/Resource.inl"
#include "Texture.h"

using namespace L;

static Symbol light_symbol("light"), present_symbol("present");

Pipeline::Pipeline(const Parameters& parameters) {
  { // Make sure shader bindings are not incompatible
    for(const auto& shader_pair : parameters.shaders) {
      const Shader* shader = shader_pair.value().force_load();
      for(const ShaderBinding& binding : shader->bindings()) {
        bool already_bound = false;
        for(ShaderBinding& own_binding : _bindings) {
          if(binding.name == own_binding.name && binding.binding == own_binding.binding) {
            own_binding.stage = binding.stage;
            already_bound = true;
            break;
          }
        }
        if(!already_bound) {
          _bindings.push(binding);
        }
      }
    }
  }

  BlendMode blend_mode = parameters.blend_mode;

  { // Determine render pass from name
    if(parameters.render_pass == light_symbol) {
      _render_pass = Renderer::get()->get_light_pass();
      if(blend_mode == BlendMode::Undefined) {
        blend_mode = BlendMode::Add;
      }
    } else if(parameters.render_pass == present_symbol) {
      _render_pass = Renderer::get()->get_present_pass();
      if(blend_mode == BlendMode::Undefined) {
        blend_mode = BlendMode::Alpha;
      }
    } else { // Default to geometry pass
      _render_pass = Renderer::get()->get_geometry_pass();
      if(blend_mode == BlendMode::Undefined) {
        blend_mode = BlendMode::None;
      }
    }
  }

  Array<ShaderImpl*> shaders;

  for(const auto& pair : parameters.shaders) {
    shaders.push(pair.value().force_load()->get_impl());
  }

  _impl = Renderer::get()->create_pipeline(
    shaders.begin(),
    shaders.size(),
    _bindings.begin(),
    _bindings.size(),
    parameters.vertex_attributes.begin(),
    parameters.vertex_attributes.size(),
    _render_pass,
    parameters.polygon_mode != PolygonMode::Undefined ? parameters.polygon_mode : PolygonMode::Fill,
    parameters.cull_mode != CullMode::Undefined ? parameters.cull_mode : CullMode::Back,
    parameters.topology != PrimitiveTopology::Undefined ? parameters.topology : PrimitiveTopology::TriangleList,
    blend_mode,
    parameters.depth_func != DepthFunc::Undefined ? parameters.depth_func : DepthFunc::Less);
}
Pipeline::~Pipeline() {
  Renderer::get()->destroy_pipeline(_impl);
}

const ShaderBinding* Pipeline::find_binding(const Symbol& name) const {
  for(const ShaderBinding& binding : _bindings) {
    if(binding.name == name) {
      return &binding;
    }
  }
  return nullptr;
}
const ShaderBinding* Pipeline::find_binding(int32_t binding_index) const {
  for(const ShaderBinding& binding : _bindings) {
    if(binding.type == ShaderBindingType::Uniform && binding.binding == binding_index) {
      return &binding;
    }
  }
  return nullptr;
}

bool Pipeline::set_descriptor(int32_t index, DescriptorSetImpl* desc_set, const UniformBuffer& buffer) const {
  if(const ShaderBinding* binding = find_binding(index)) {
    Renderer::get()->update_descriptor_set(desc_set, *binding, buffer.get_impl());
    return true;
  } else {
    return false;
  }
}
bool Pipeline::set_descriptor(int32_t index, DescriptorSetImpl* desc_set, const Texture& texture) const {
  if(const ShaderBinding* binding = find_binding(index)) {
    Renderer::get()->update_descriptor_set(desc_set, *binding, texture.get_impl());
    return true;
  } else {
    return false;
  }
}
bool Pipeline::set_descriptor(const Symbol& name, DescriptorSetImpl* desc_set, const UniformBuffer& buffer) const {
  if(const ShaderBinding* binding = find_binding(name)) {
    Renderer::get()->update_descriptor_set(desc_set, *binding, buffer.get_impl());
    return true;
  } else {
    return false;
  }
}
bool Pipeline::set_descriptor(const Symbol& name, DescriptorSetImpl* desc_set, const Texture& texture) const {
  if(const ShaderBinding* binding = find_binding(name)) {
    Renderer::get()->update_descriptor_set(desc_set, *binding, texture.get_impl());
    return true;
  } else {
    return false;
  }
}
bool Pipeline::set_descriptor(const Symbol& name, DescriptorSetImpl* desc_set, FramebufferImpl* framebuffer, int32_t texture_index) const {
  if(const ShaderBinding* binding = find_binding(name)) {
    Renderer::get()->update_descriptor_set(desc_set, *binding, framebuffer, texture_index);
    return true;
  } else {
    return false;
  }
}
