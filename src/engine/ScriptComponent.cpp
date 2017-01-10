#include "ScriptComponent.h"

#include "../streams/FileStream.h"
#include "Camera.h"
#include "Collider.h"
#include "Sprite.h"
#include "Transform.h"
#include "StaticMesh.h"
#include "Primitive.h"
#include "../text/String.h"

using namespace L;
using namespace Script;

void ScriptComponent::updateComponents(){
  static Var updateComponentsCall(Array<Var>{Symbol("update-components")});
  _context.selfTable()[Symbol("entity")] = entity();
  _context.execute(updateComponentsCall);
}
void ScriptComponent::load(const char* filename) {
  static Var startCall(Array<Var>(1, Var(Array<Var>{Symbol("self"), Script::Quote{Symbol("start")}})));
  FileStream stream(filename,"rb");
  _context.read(stream);
  _context.execute(startCall);
}
void ScriptComponent::update() {
  static Var updateCall(Array<Var>(1, Var(Array<Var>{Symbol("self"), Script::Quote{Symbol("update")}})));
  _context.execute(updateCall);
}
void ScriptComponent::lateUpdate() {
  static Var updateCall(Array<Var>(1,Var(Array<Var>{Symbol("self"), Script::Quote{Symbol("late-update")}})));
  _context.execute(updateCall);
}
void ScriptComponent::event(const Device::Event& e){
  auto table(ref<Table<Var,Var>>());
  (*table)[Symbol("device")] = e._device;
  (*table)[Symbol("index")] = (int)e._index;
  (*table)[Symbol("pressed")] = (bool)e._pressed;
  event(table);
}
void ScriptComponent::event(const Window::Event& e){
  auto table(ref<Table<Var,Var>>());
  Var& typeSlot((*table)[Symbol("type")]);
  switch(e.type){ // TODO: handle other event types
    case Window::Event::MOUSEMOVE:
      typeSlot = Symbol("MOUSEMOVE");
      break;
    case Window::Event::BUTTONDOWN:
      typeSlot = Symbol("BUTTONDOWN");
      break;
    case Window::Event::BUTTONUP:
      typeSlot = Symbol("BUTTONUP");
      break;
  }
  (*table)[Symbol("button")] = Window::buttonToSymbol(e.button);
  (*table)[Symbol("x")] = e.x;
  (*table)[Symbol("y")] = e.y;
  event(table);
}
void ScriptComponent::event(const Ref<Table<Var,Var>>&e){
  Var eventCall(Array<Var>{Symbol("event"),e});
  _context.execute(eventCall);
}

void ScriptComponent::init() {
  L_ONCE;
#define L_FUNCTION(name,...) Context::global(Symbol(name)) = (Function)([](const Var& src,SymbolVar* stack,size_t params)->Var {__VA_ARGS__ return 0;})
#define L_COMPONENT_FUNCTION(cname,fname,n,...) Context::typeValue(Type<cname*>::description(),Symbol(fname)) = (Function)([](const Var& src,SymbolVar* stack,size_t params)->Var {L_ASSERT(params>=n && src.is<cname*>());__VA_ARGS__ return 0;})
#define L_COMPONENT_METHOD(cname,fname,n,...) L_COMPONENT_FUNCTION(cname,fname,n,src.as<cname*>()->__VA_ARGS__;)
#define L_COMPONENT_RETURN_METHOD(cname,fname,n,...) L_COMPONENT_FUNCTION(cname,fname,n,return src.as<cname*>()->__VA_ARGS__;)
#define L_COMPONENT_ADD(cname,fname) L_COMPONENT_FUNCTION(Entity,fname,0,return src.as<Entity*>()->add<cname>();)
#define L_COMPONENT_GET(cname,fname) L_COMPONENT_FUNCTION(Entity,fname,0,return src.as<Entity*>()->component<cname>();)
#define L_COMPONENT_REQUIRE(cname,fname) L_COMPONENT_FUNCTION(Entity,fname,0,return src.as<Entity*>()->requireComponent<cname>();)
#define L_COMPONENT_COPY(cname) L_COMPONENT_FUNCTION(cname,"copy",1,if(stack[0]->is<cname*>())*(src.as<cname*>()) = *(stack[0]->as<cname*>());)
#define L_COMPONENT_ENTITY(cname) L_COMPONENT_RETURN_METHOD(cname,"entity",0,entity())
#define L_COMPONENT_BIND(cname,name)\
  L_COMPONENT_ADD(cname,"add-" name);\
  L_COMPONENT_GET(cname,"get-" name);\
  L_COMPONENT_REQUIRE(cname,"require-" name);\
  L_COMPONENT_ENTITY(cname);\
  L_COMPONENT_COPY(cname);\
  Type<cname*>::cancmp<>();
  // Engine ///////////////////////////////////////////////////////////////////
  L_FUNCTION("engine-timescale",{
    if(params>0)
      Engine::timescale(stack[0]->get<float>());
    return Engine::timescale();
  });
  Context::global(Symbol("engine-gravity")) = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    if(params>0)
      RigidBody::gravity(stack[0]->get<Vector3f>());
    return RigidBody::gravity();
  });
  // Entity ///////////////////////////////////////////////////////////////////
  Context::global(Symbol("entity-make")) = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    return new Entity();
  });
  Context::global(Symbol("entity-copy")) = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    if(params && stack[0]->is<Entity*>())
      return new Entity(stack[0]->as<Entity*>());
    return 0;
  });
  Context::global(Symbol("entity-destroy")) = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    if(params && stack[0]->is<Entity*>())
      Entity::destroy(stack[0]->as<Entity*>());
    return 0;
  });
  // Devices ///////////////////////////////////////////////////////////////////
  L_FUNCTION("get-devices",{
    auto wtr(ref<Table<Var,Var>>());
    for(auto&& device : Device::devices())
      (*wtr)[&device] = true;
    return wtr;
  });
  L_COMPONENT_RETURN_METHOD(const Device,"get-axis",1,axis(stack[0]->get<int>()));
  L_COMPONENT_RETURN_METHOD(const Device,"get-button",1,button(stack[0]->get<int>()));
  // Transform ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Transform,"transform");
  L_COMPONENT_RETURN_METHOD(Transform,"get-position",0,absolutePosition());
  L_COMPONENT_RETURN_METHOD(Transform,"get-translation",0,translation());
  L_COMPONENT_RETURN_METHOD(Transform,"right",0,right());
  L_COMPONENT_RETURN_METHOD(Transform,"forward",0,forward());
  L_COMPONENT_RETURN_METHOD(Transform,"up",0,up());
  L_COMPONENT_METHOD(Transform,"set-translation",1,translation(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(Transform,"move",1,move(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(Transform,"rotate",2,rotate(stack[0]->get<Vector3f>(),stack[1]->get<float>()));
  L_COMPONENT_METHOD(Transform,"scale",1,scale(stack[0]->get<float>()));
  // Collider ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Collider,"collider");
  L_COMPONENT_METHOD(Collider,"center",1,center(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(Collider,"box",1,box(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(Collider,"sphere",1,sphere(stack[0]->get<float>()));
  Context::global(Symbol("raycast")) = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    if(params==2 && stack[0]->is<Vector3f>() && stack[1]->is<Vector3f>()){
      auto wtr(ref<Table<Var,Var>>());
      float t;
      (*wtr)[Symbol("collider")] = Collider::raycast(stack[0]->as<Vector3f>(),stack[1]->as<Vector3f>(),t);
      (*wtr)[Symbol("t")] = t;
      (*wtr)[Symbol("position")] = stack[0]->as<Vector3f>()+stack[1]->as<Vector3f>()*t;
      return wtr;
    }
    return nullptr;
  });
  // RigidBody ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(RigidBody,"rigidbody");
  L_COMPONENT_METHOD(RigidBody,"mass",1,mass(stack[0]->get<float>()));
  L_COMPONENT_METHOD(RigidBody,"restitution",1,restitution(stack[0]->get<float>()));
  L_COMPONENT_METHOD(RigidBody,"drag",1,drag(stack[0]->get<float>()));
  L_COMPONENT_METHOD(RigidBody,"angular-drag",1,angularDrag(stack[0]->get<float>()));
  L_COMPONENT_RETURN_METHOD(RigidBody, "get-speed", 0, velocity());
  L_COMPONENT_RETURN_METHOD(RigidBody, "get-relative-speed", 0, relativeVelocity());
  L_COMPONENT_METHOD(RigidBody,"add-speed",1,addSpeed(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(RigidBody,"add-force",1,addForce(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(RigidBody,"add-relative-force",1,addRelativeForce(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(RigidBody,"add-torque",1,addTorque(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(RigidBody,"add-relative-torque",1,addRelativeTorque(stack[0]->get<Vector3f>()));
  // Camera ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Camera,"camera");
  L_COMPONENT_METHOD(Camera,"perspective",3,perspective(stack[0]->get<float>(),stack[1]->get<float>(),stack[2]->get<float>()));
  L_COMPONENT_METHOD(Camera,"ortho",4,ortho(stack[0]->get<float>(),stack[1]->get<float>(),stack[2]->get<float>(),stack[3]->get<float>()));
  L_COMPONENT_METHOD(Camera,"viewport",4,viewport(Interval2f(Vector2f(stack[0]->get<float>(),stack[1]->get<float>()),Vector2f(stack[2]->get<float>(),stack[3]->get<float>()))));
  // Script ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(ScriptComponent,"script");
  L_COMPONENT_METHOD(ScriptComponent,"load",1,load(stack[0]->get<String>()));
  L_COMPONENT_FUNCTION(ScriptComponent,"call",1,{
    L_ASSERT(stack[0]->is<Symbol>());
    Array<Var> code(Array<Var>(1, Var(Array<Var>{Symbol("self"), Script::Quote{stack[0]->as<Symbol>()}})));
    for(uintptr_t i(1); i<params; i++)
      code.push(stack[i].value());
    return src.as<ScriptComponent*>()->_context.execute(code);
  });
  // Sprite ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Sprite,"sprite");
  L_COMPONENT_METHOD(Sprite,"load",1,texture(stack[0]->get<String>()));
  // StaticMesh ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(StaticMesh,"staticmesh");
  L_COMPONENT_METHOD(StaticMesh,"mesh",1,mesh((const char*)stack[0]->get<String>()));
  L_COMPONENT_METHOD(StaticMesh,"texture",1,texture((const char*)stack[0]->get<String>()));
  // Primitive ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(Primitive,"primitive");
  L_COMPONENT_METHOD(Primitive,"center",1,center(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(Primitive,"box",1,box(stack[0]->get<Vector3f>()));
  L_COMPONENT_METHOD(Primitive,"sphere",1,sphere(stack[0]->get<float>()));
  L_COMPONENT_METHOD(Primitive,"color",1,color(stack[0]->get<Color>()));
}
