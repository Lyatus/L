#pragma once

#include <L/src/L.h>

namespace L {
  class OBJ : public Interface<Mesh> {
    static OBJ instance;
  private:
    struct Vertex {
      Vector3f _vertex;
      Vector2f _uv;
      Vector3f _normal;
    };

  public:
    OBJ() : Interface{"obj"} {}

    Ref<Mesh> from(Stream& stream) override {
      Array<Vector3f> _vertices, _normals;
      Array<Vector2f> _uvs;
      MeshBuilder mb;
      while(!stream.end()) {
        char buffer[1024];
        stream.line(buffer, sizeof(buffer));
        const Array<String> line(String(buffer).explode(' '));
        if(line.empty()) continue;
        else if(line[0]=="v") _vertices.push(atof(line[1]), atof(line[2]), atof(line[3]));
        else if(line[0]=="vt") _uvs.push(atof(line[1]), 1.f-atof(line[2])); // OpenGL has y going bottom up
        else if(line[0]=="vn") _normals.push(atof(line[1]), atof(line[2]), atof(line[3]));
        else if(line[0]=="f") {
          Vertex vertex{}, firstVertex{}, lastVertex{};
          for(uintptr_t i(1); i<line.size(); i++) {
            const Array<String> indices(line[i].explode('/'));

            switch(indices.size()) {
              case 3: vertex._normal = _normals[atoi(indices[2])-1];
              case 2: if(!_uvs.empty())vertex._uv = _uvs[atoi(indices[1])-1];
                      else vertex._normal = _normals[atoi(indices[1])-1];
              case 1: vertex._vertex = _vertices[atoi(indices[0])-1];
            }

            if(i==1) firstVertex = vertex;
            else if(i>3) {
              mb.addVertex(&firstVertex, sizeof(Vertex));
              mb.addVertex(&lastVertex, sizeof(Vertex));
            }
            mb.addVertex(&vertex, sizeof(Vertex));
            lastVertex = vertex;
          }
        }
      }
      if(_normals.empty())
        mb.computeNormals(0, sizeof(Vector2f)+sizeof(Vector3f), sizeof(Vertex));

      static const std::initializer_list<Mesh::Attribute> attributes = {
        {3,GL_FLOAT,GL_FALSE},
        {2,GL_FLOAT,GL_FALSE},
        {3,GL_FLOAT,GL_FALSE},
      };
      L_SCOPE_THREAD_MASK(1); // Go to main thread
      return ref<Mesh>(mb, GL_TRIANGLES, attributes);
    }
  };
  OBJ OBJ::instance;
}
