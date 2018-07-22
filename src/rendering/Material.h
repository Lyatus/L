#pragma once

#include "../container/KeyValue.h"
#include "../engine/Resource.h"
#include "../image/Color.h"
#include "Pipeline.h"
#include "Texture.h"
#include "Mesh.h"
#include "DescriptorSet.h"

namespace L {
  class Material {
  protected:
    Resource<Material> _parent;
    Resource<Pipeline> _pipeline;
    Resource<Mesh> _mesh;
    Array<KeyValue<Symbol, float>> _scalars;
    Array<KeyValue<Symbol, Resource<Texture>>> _textures;
    Array<KeyValue<Symbol, Vector4f>> _vectors;
    DescriptorSet* _desc_set = nullptr;
    size_t _vertex_count = 0;
  public:
    typedef Material* Intermediate;
    void draw(VkCommandBuffer cmd_buffer, const Matrix44f& model = Matrix44f(1.f));
    bool valid() const;
    Interval3f bounds() const;
    void make_desc_set();

    inline void parent(const Resource<Material>& parent) { _parent = parent; }
    inline void pipeline(const Resource<Pipeline>& pipeline) { _pipeline = pipeline; }
    inline void mesh(const Resource<Mesh>& mesh) { _mesh = mesh; }
    void scalar(const Symbol& name, float scalar, bool override = true);
    void texture(const Symbol& name, const Resource<Texture>& texture, bool override = true);
    void vector(const Symbol& name, const Vector4f& vector, bool override = true);
    inline void color(const Symbol& name, const Color& color) { vector(name, Color::to_float_vector(color)); }
    inline void vertex_count(size_t count) { _vertex_count = count; }

    inline Resource<Pipeline> final_pipeline() const { return _pipeline.is_set() ? _pipeline : (_parent.is_set() ? _parent->final_pipeline() : _pipeline); }
    inline Resource<Mesh> final_mesh() const { return _mesh.is_set() ? _mesh : (_parent.is_set() ? _parent->final_mesh() : _mesh); }
    inline const DescriptorSet* final_desc_set() const { return _desc_set? _desc_set : (_parent.is_set() ? _parent->final_desc_set() : _desc_set); }
    inline size_t final_vertex_count() const { return _vertex_count ? _vertex_count : (_parent.is_set() ? _parent->final_vertex_count() : _vertex_count); }
  };
}
