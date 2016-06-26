#include "ScriptComponent.h"

#include "../streams/FileStream.h"
#include "Camera.h"
#include "Collider.h"
#include "Sprite.h"
#include "Transform.h"

using namespace L;
using namespace Script;

void ScriptComponent::start(){
  _context.local(FNV1A("entity")) = entity();
}
void ScriptComponent::load(const char* filename) {
  static Var startCall(Array<Var>{FNV1A("start")});
  FileStream stream(filename,"rb");
  _context.read(stream);
  _context.execute(startCall);
}
void ScriptComponent::update() {
  static Var updateCall(Array<Var>{FNV1A("update")});
  _context.variable("delta") = Engine::deltaSeconds();
  _context.execute(updateCall);
}

void ScriptComponent::init() {
  L_ONCE;
#define L_FUNCTION(name,...) Context::global(FNV1A(name)) = (Function)([](SymbolVar* stack,size_t params)->Var {__VA_ARGS__ return 0;})
#define L_COMPONENT_FUNCTION(cname,fname,n,...) L_FUNCTION(fname,if(params>=n && stack[0]->is<cname*>()){__VA_ARGS__})
#define L_COMPONENT_METHOD(cname,fname,n,...) L_COMPONENT_FUNCTION(cname,fname,n,stack[0]->as<cname*>()->__VA_ARGS__;)
#define L_COMPONENT_ADD(cname,fname) L_FUNCTION(fname,\
    if(params==1 && stack[0]->is<Entity*>())\
      return stack[0]->as<Entity*>()->add<cname>();)
#define L_COMPONENT_GET(cname,fname) L_FUNCTION(fname,\
    if(params==1 && stack[0]->is<Entity*>())\
      return stack[0]->as<Entity*>()->component<cname>();)
#define L_COMPONENT_REQUIRE(cname,fname) L_FUNCTION(fname,\
    if(params==1 && stack[0]->is<Entity*>())\
      return stack[0]->as<Entity*>()->requireComponent<cname>();)
#define L_COMPONENT_BIND(cname,name) L_COMPONENT_ADD(cname,name "-add"); L_COMPONENT_GET(cname,name "-get"); L_COMPONENT_REQUIRE(cname,name "-require");
  // Engine ///////////////////////////////////////////////////////////////////
  L_FUNCTION("engine-timescale",{
    if(params>0)
      Engine::timescale(stack[0]->get<float>());
    return Engine::timescale();
  });
  // Entity ///////////////////////////////////////////////////////////////////
  Context::global(FNV1A("entity-make")) = (Function)([](SymbolVar* stack,size_t params)->Var {
    return new Entity();
  });
  Context::global(FNV1A("entity-copy")) = (Function)([](SymbolVar* stack,size_t params)->Var {
    if(params && stack[0]->is<Entity*>())
      return new Entity(stack[0]->as<Entity*>());
    return 0;
  });
  Context::global(FNV1A("entity-destroy")) = (Function)([](SymbolVar* stack,size_t params)->Var {
    if(params && stack[0]->is<Entity*>())
      delete stack[0]->as<Entity*>();
    return 0;
  });
  // Transform ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Transform,"transform");
  L_COMPONENT_METHOD(Transform,"transform-move",2,move(stack[1]->get<Vector3f>()));
  L_COMPONENT_METHOD(Transform,"transform-rotate",3,rotate(stack[1]->get<Vector3f>(),stack[2]->get<float>()));
  L_COMPONENT_FUNCTION(Transform,"transform-copy",2,{
    if(stack[1]->is<Transform*>()){
      Transform& a(*stack[0]->as<Transform*>());
      const Transform& b(*stack[1]->as<Transform*>());
      a.parent(b.parent());
      a.translation(b.translation());
      a.rotation(b.rotation());
    }
  });
  // Collider ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Collider,"collider");
  L_COMPONENT_FUNCTION(Collider,"collider-box",1,{
    Vector3f size(1.f,1.f,1.f);
    if(params == 2 && stack[1]->is<Vector3f>())
      size = stack[1]->get<Vector3f>();
    stack[0]->as<Collider*>()->box(Interval3f(-size/2.f,size/2.f));
  });
  // RigidBody ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(RigidBody,"rigidbody");
  L_COMPONENT_METHOD(RigidBody,"rigidbody-addspeed",2,addSpeed(stack[1]->get<Vector3f>()));
  // Camera ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Camera,"camera");
  L_COMPONENT_METHOD(Camera,"camera-perspective",5,perspective(stack[1]->get<float>(),stack[2]->get<float>(),stack[3]->get<float>(),stack[4]->get<float>()));
  // Script ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(ScriptComponent,"script");
  L_COMPONENT_METHOD(ScriptComponent,"script-load",2,load(stack[1]->get<String>()));
  // Sprite ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Sprite,"sprite");
  L_COMPONENT_METHOD(Sprite,"sprite-load",2,texture((const char*)stack[1]->get<String>()));
}