#include "GL.h"

#include "../constants.h"
#include "MeshBuilder.h"
#include "Shader.h"
#include "../image/Color.h"
#include "../engine/SharedUniform.h"

using namespace L;
using namespace GL;

MeshBuilder meshBuilder;

const Texture& GL::whiteTexture() {
  static Texture tex(1,1,&Color::white);
  return tex;
}
#define L_VERTEX_SHADER \
"#version 330 core\n" \
L_SHAREDUNIFORM \
"layout (location = 0) in vec3 modelPosition;" \
"uniform mat4 model;" \
"smooth out vec4 position;" \
"void main(){" \
  "position = model * vec4(modelPosition,1.0);" \
  "gl_Position = viewProj * position;" \
"}"
Program& GL::baseProgram() {
  static Program program(Shader(L_VERTEX_SHADER,GL_VERTEX_SHADER),Shader(
    "#version 330 core\n"
    L_SHAREDUNIFORM
    "layout(location = 0) out vec3 ocolor;"
    "layout(location = 1) out vec3 onormal;"
    "smooth in vec4 position;"
    "void main(){"
    "vec3 normal = cross(dFdx(position.xyz),dFdy(position.xyz)).xyz;"
    "if(isnan(normal.x) || length(normal)<=0.f) normal = eye.xyz-position.xyz;"
    "onormal.xy = encodeNormal(normal);"
    "ocolor = vec3(1,1,1);"
    "}",GL_FRAGMENT_SHADER));
  return program;
}
Program& GL::baseColorProgram() {
  static Program program(Shader(L_VERTEX_SHADER,GL_VERTEX_SHADER),Shader(
    "#version 330 core\n"
    L_SHAREDUNIFORM
    "layout(location = 0) out vec3 ocolor;"
    "layout(location = 1) out vec3 onormal;"
    "uniform vec4 color;"
    "smooth in vec4 position;"
    "void main(){"
    "if(alpha(color.a)) discard;"
    "vec3 normal = cross(dFdx(position.xyz),dFdy(position.xyz)).xyz;"
    "if(isnan(normal.x) || length(normal)<=0.f) normal = eye.xyz-position.xyz;"
    "onormal.xy = encodeNormal(normal);"
    "ocolor = color.rgb;"
    "}",GL_FRAGMENT_SHADER));
  return program;
}
const char* GL::error() {
  static char invalidEnum[] = "GL_INVALID_ENUM";
  static char invalidValue[] = "GL_INVALID_VALUE";
  static char invalidOperation[] = "GL_INVALID_OPERATION";
  static char invalidFramebufferOperation[] = "GL_INVALID_FRAMEBUFFER_OPERATION";
  static char outOfMemory[] = "GL_OUT_OF_MEMORY";
  GLenum error(glGetError());
  switch(error) {
    case GL_INVALID_ENUM:
      return invalidEnum;
    case GL_INVALID_VALUE:
      return invalidValue;
    case GL_INVALID_OPERATION:
      return invalidOperation;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return invalidFramebufferOperation;
    case GL_OUT_OF_MEMORY:
      return outOfMemory;
    default:
      return nullptr;
  }
}
const Mesh& GL::quad(){
  static const GLfloat quad[] = {
    -1,-1,0,
    1,-1,0,
    -1,1,0,
    1,1,0,
  };
  static Mesh mesh(GL_TRIANGLE_STRIP,4,quad,sizeof(quad),{Mesh::Attribute{0,3,GL_FLOAT,GL_FALSE,0,0}});
  return mesh;
}
const Mesh& GL::cube(){
  static const GLfloat cube[] = {
    // Bottom face
    -1,-1,-1, -1,1,-1,  1,-1,-1,
    -1,1,-1,  1,1,-1,   1,-1,-1,
    // Top face
    -1,-1,1,  1,-1,1,   -1,1,1,
    -1,1,1,   1,-1,1,   1,1,1,
    // Back face
    -1,-1,-1, 1,-1,-1,  -1,-1,1,
    1,-1,-1,  1,-1,1,   -1,-1,1,
    // Front face
    -1,1,-1,  -1,1,1,   1,1,-1,
    1,1,-1,   -1,1,1,   1,1,1,
    // Left face
    -1,-1,-1, -1,-1,1,  -1,1,-1,
    -1,-1,1,  -1,1,1,   -1,1,-1,
    // Right face
    1,-1,-1,  1,1,-1,   1,-1,1,
    1,-1,1,   1,1,-1,   1,1,1,
  };
  static Mesh mesh(GL_TRIANGLES,6*2*3,cube,sizeof(cube),{Mesh::Attribute{0,3,GL_FLOAT,GL_FALSE,0,0}});
  return mesh;
}
const Mesh& GL::wireCube(){
  static const GLfloat wireCube[] = {
    // Bottom face
    -1,-1,-1, -1,1,-1,
    -1,-1,-1, 1,-1,-1,
    1,-1,-1,  1,1,-1,
    -1,1,-1,  1,1,-1,
    // Top face
    -1,-1,1, -1,1,1,
    -1,-1,1, 1,-1,1,
    1,-1,1,  1,1,1,
    -1,1,1,  1,1,1,
    // Sides
    -1,-1,-1, -1,-1,1,
    -1,1,-1,  -1,1,1,
    1,-1,-1,  1,-1,1,
    1,1,-1,   1,1,1,
  };
  static Mesh mesh(GL_LINES,12*2,wireCube,sizeof(wireCube),{Mesh::Attribute{0,3,GL_FLOAT,GL_FALSE,0,0}});
  return mesh;
}
const Mesh& GL::sphere(){
  static const float d(sqrt(.5f));
  static const GLfloat sphere[] = {
    // -X-Y-Z octant
    0,0,-1,  0,-d,-d, -d,0,-d,
    0,-1,0,  -d,-d,0, 0,-d,-d,
    -1,0,0,  -d,0,-d, -d,-d,0,
    0,-d,-d, -d,-d,0, -d,0,-d,
    // -X-Y+Z octant
    0,0,1,   -d,0,d,  0,-d,d,
    0,-1,0,  0,-d,d,  -d,-d,0,
    -1,0,0,  -d,-d,0, -d,0,d,
    0,-d,d,  -d,0,d,  -d,-d,0,
    // -X+Y-Z octant
    0,0,-1,  -d,0,-d, 0,d,-d,
    0,1,0,   0,d,-d,  -d,d,0,
    -1,0,0,  -d,d,0,  -d,0,-d,
    0,d,-d,  -d,0,-d, -d,d,0,
    // -X+Y+Z octant
    0,0,1,   0,d,d,   -d,0,d,
    0,1,0,   -d,d,0,  0,d,d,
    -1,0,0,  -d,0,d,  -d,d,0,
    0,d,d,   -d,d,0,  -d,0,d,
    // +X-Y-Z octant
    0,0,-1,  d,0,-d,  0,-d,-d,
    0,-1,0,  0,-d,-d, d,-d,0,
    1,0,0,   d,-d,0,  d,0,-d,
    0,-d,-d, d,0,-d,  d,-d,0,
    // +X-Y+Z octant
    0,0,1,   0,-d,d,  d,0,d,
    0,-1,0,  d,-d,0,  0,-d,d,
    1,0,0,   d,0,d,   d,-d,0,
    0,-d,d,  d,-d,0,  d,0,d,
    // +X+Y-Z octant
    0,0,-1,  0,d,-d,  d,0,-d,
    0,1,0,   d,d,0,   0,d,-d,
    1,0,0,   d,0,-d,  d,d,0,
    0,d,-d,  d,d,0,   d,0,-d,
    // +X+Y+Z octant
    0,0,1,  d,0,d, 0,d,d,
    0,1,0,  0,d,d, d,d,0,
    1,0,0,  d,d,0, d,0,d,
    0,d,d,  d,0,d, d,d,0,
  };
  static Mesh mesh(GL_TRIANGLES,8*4*3,sphere,sizeof(sphere),{Mesh::Attribute{0,3,GL_FLOAT,GL_FALSE,0,0}});
  return mesh;
}
const Mesh& GL::wireSphere(){
  static const float d(sqrt(.5f));
  static const GLfloat wireSphere[] = {
    // X circle
    0,0,-1, 0,-d,-d, 0,-d,-d, 0,-1,0,
    0,-1,0, 0,-d,d,  0,-d,d,  0,0,1,
    0,0,1,  0,d,d,   0,d,d,   0,1,0,
    0,1,0,  0,d,-d,  0,d,-d,  0,0,-1,
    // Y circle
    0,0,-1, -d,0,-d, -d,0,-d, -1,0,0,
    -1,0,0, -d,0,d,  -d,0,d,  0,0,1,
    0,0,1,  d,0,d,   d,0,d,   1,0,0,
    1,0,0,  d,0,-d,  d,0,-d,  0,0,-1,
    // Z circle
    0,-1,0, -d,-d,0, -d,-d,0, -1,0,0,
    -1,0,0, -d,d,0,  -d,d,0,  0,1,0,
    0,1,0,  d,d,0,   d,d,0,   1,0,0,
    1,0,0, d,-d,0,   d,-d,0,  0,-1,0,
  };
  static Mesh mesh(GL_LINES,24*2,wireSphere,sizeof(wireSphere),{Mesh::Attribute{0,3,GL_FLOAT,GL_FALSE,0,0}});
  return mesh;
}
void GL::makeDisc(Mesh& mesh,int slices) {
  /*
  meshBuilder.reset(Mesh::VERTEX);
  meshBuilder.setVertex(Vector3f(0,0,0));
  uint32_t center(meshBuilder.addVertex());
  meshBuilder.setVertex(Vector3f(1,0,0));
  uint32_t first(meshBuilder.addVertex());
  uint32_t last(first);
  for(int i(1); i<slices; i++) {
    float angle(((float)i/slices)*PI<float>()*2);
    meshBuilder.setVertex(Vector3f(cos(angle),sin(angle),0));
    uint32_t current(meshBuilder.addVertex());
    meshBuilder.addTriangle(center,current,last);
    last = current;
  }
  meshBuilder.addTriangle(center,first,last);
  reconstruct(mesh,meshBuilder);
  */
}
Map<uint64_t,int> middles;
int vertexBetween(MeshBuilder& mb,uint32_t a,uint32_t b) {
  /*
  if(a>b) swap(a,b);
  uint64_t id((uint64_t)a<<32|b);
  if(!middles.has(id)) {
    Vector3f va(mb.vertex(a)),vb(mb.vertex(b));
    Vector3f v((va+vb)/2.f);
    v.normalize();
    mb.setVertex(v);
    middles[id] = mb.addVertex();
  }
  return middles[id];
  */
  return 0;
}
void refineTriangle(MeshBuilder& mb,uint32_t a,uint32_t b,uint32_t c,uint32_t rec) {
  /*
  if(rec) {
    int ab(vertexBetween(mb,a,b)),bc(vertexBetween(mb,b,c)),ac(vertexBetween(mb,a,c));
    rec--;
    refineTriangle(mb,a,ab,ac,rec);
    refineTriangle(mb,b,bc,ab,rec);
    refineTriangle(mb,c,ac,bc,rec);
    refineTriangle(mb,ab,bc,ac,rec);
  } else mb.addTriangle(a,b,c);
  */
}
void GL::makeSphere(Mesh& mesh,int rec) {
  /*
  middles.clear();
  meshBuilder.reset(Mesh::VERTEX);
  meshBuilder.setVertex(Vector3f(-1,0,0));
  meshBuilder.addVertex();
  meshBuilder.setVertex(Vector3f(1,0,0));
  meshBuilder.addVertex();
  meshBuilder.setVertex(Vector3f(0,-1,0));
  meshBuilder.addVertex();
  meshBuilder.setVertex(Vector3f(0,1,0));
  meshBuilder.addVertex();
  meshBuilder.setVertex(Vector3f(0,0,-1));
  meshBuilder.addVertex();
  meshBuilder.setVertex(Vector3f(0,0,1));
  meshBuilder.addVertex();
  refineTriangle(meshBuilder,0,2,5,rec);
  refineTriangle(meshBuilder,0,3,4,rec);
  refineTriangle(meshBuilder,0,4,2,rec);
  refineTriangle(meshBuilder,0,5,3,rec);
  refineTriangle(meshBuilder,1,2,4,rec);
  refineTriangle(meshBuilder,1,3,5,rec);
  refineTriangle(meshBuilder,1,4,3,rec);
  refineTriangle(meshBuilder,1,5,2,rec);
  reconstruct(mesh,meshBuilder);
  */
}