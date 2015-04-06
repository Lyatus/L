#ifndef DEF_L_Interface_obj
#define DEF_L_Interface_obj

#include <L/L.h>

namespace L {
  class OBJ : public Interface<GL::Mesh> {
    public:
      OBJ() : Interface("obj") {}

      bool from(GL::Mesh& mesh, const File& f) {
        GL::MeshBuilder mb;
        std::ifstream file(f.path().c_str(), std::ios::in);
        if(file) {
          String line;
          while(getline(file, line)) {
            List<String> linePart(line.explode(' '));
            if(linePart[0]=="v") { // Vertex
              Point3f tmp;
              tmp.x() = FromString<double>(linePart[1]);
              tmp.y() = FromString<double>(linePart[2]);
              tmp.z() = FromString<double>(linePart[3]);
              mb.setVertex(tmp);
              mb.addVertex();
            }/* else if(linePart[0]=="vt") { // TexCoord
              Point2f tmp;
              tmp.x() = FromString<double>(linePart[1]);
              tmp.y() = FromString<double>(linePart[2]);
              mesh.texCoord.push_back(tmp);
            } else if(linePart[0]=="vn") { // Normal
              Point3f tmp;
              tmp.x() = FromString<double>(linePart[1]);
              tmp.y() = FromString<double>(linePart[2]);
              tmp.z() = FromString<double>(linePart[3]);
              mesh.normal.push_back(tmp);
            }*/ else if(linePart[0]=="f") { // Face abc:vertex; def:tex; g:normal;
              if(1) {
                List<String> linePartPart(linePart[1].explode('/'));
                mb.addIndex(FromString<size_t>(linePartPart[0])-1);
                //tmp.t1 = FromString<size_t>(linePartPart[1])-1;
              }
              if(1) {
                List<String> linePartPart(linePart[2].explode('/'));
                mb.addIndex(FromString<size_t>(linePartPart[0])-1);
                //tmp.t2 = FromString<size_t>(linePartPart[1])-1;
              }
              if(1) {
                List<String> linePartPart(linePart[3].explode('/'));
                mb.addIndex(FromString<size_t>(linePartPart[0])-1);
                //tmp.t3 = FromString<size_t>(linePartPart[1])-1;
                //tmp.n = FromString<size_t>(linePartPart[2])-1;
              }
            }
          }
          mb.computeNormals();
          L_Reconstruct(GL::,Mesh,mesh,(mb));
        }
        return true;
      }
  };
}

#endif

