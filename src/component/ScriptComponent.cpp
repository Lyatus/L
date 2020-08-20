#include "ScriptComponent.h"

#include "../engine/Engine.h"
#include "../rendering/Material.h"
#include "../engine/Resource.inl"
#include "../engine/Settings.h"
#include "NameComponent.h"
#include "../text/String.h"

using namespace L;

static const Symbol entity_symbol("entity"), update_components_symbol("update-components"),
start_symbol("start"), update_symbol("update"), late_update_symbol("late-update"), event_symbol("event");

void ScriptComponent::update_components() {
  _context.self_table()[entity_symbol] = entity();

  if(!_started && _script)
    start();

  _context.try_execute_method(update_components_symbol);
}
Map<Symbol, Var> ScriptComponent::pack() const {
  return {{"script",_script}};
}
void ScriptComponent::unpack(const Map<Symbol, Var>& data) {
  unpack_item(data, "script", _script);
}
void ScriptComponent::script_registration() {
  // Engine ///////////////////////////////////////////////////////////////////
  ScriptGlobal("engine_timescale") = (ScriptNativeFunction)([](ScriptContext& c) {
    if(c.param_count()>0)
      Engine::timescale(c.param(0).get<float>());
    c.return_value() = Engine::timescale();
  });
  ScriptGlobal("engine_clear") = (ScriptNativeFunction)([](ScriptContext&) {
    Engine::add_deferred_action({[](void*) {
      Engine::clear();
    }});
  });
  ScriptGlobal("engine_clear_and_read") = (ScriptNativeFunction)([](ScriptContext& c) {
    L_ASSERT(c.param_count() == 1);
    const Resource<ScriptFunction> script(c.param(0).get<String>());
    script.load();
    Engine::add_deferred_action({[](void* p) {
      Resource<ScriptFunction>* script = (Resource<ScriptFunction>*)p;
      Engine::clear();
      script->flush();
      if(script->is_loaded()) {
        ScriptContext().execute(ref<ScriptFunction>(**script));
      } else {
        warning("Could not load script %s for engine_clear_and_read", script->slot()->id);
      }
      Memory::delete_type(script);
    }, Memory::new_type<Resource<ScriptFunction>>(script)});
  });
  ScriptGlobal("read") = (ScriptNativeFunction)([](ScriptContext& c) {
    L_ASSERT(c.param_count() == 1);
    Resource<ScriptFunction> script_resource = c.param(0).get<String>();
    script_resource.flush();
    c.return_value() = script_resource.is_loaded() ? Var(ref<ScriptFunction>(*script_resource)) : Var();
  });
  ScriptGlobal("setting") = (ScriptNativeFunction)([](ScriptContext& c) {
    L_ASSERT(c.param_count()==2);
    Settings::set(c.param(0), c.param(1));
  });
  // Entity ///////////////////////////////////////////////////////////////////
  ScriptGlobal("entity_make") = (ScriptNativeFunction)([](ScriptContext& c) {
    c.return_value() = Entity::create();
  });
  ScriptGlobal("entity_copy") = (ScriptNativeFunction)([](ScriptContext& c) {
    if(c.param_count() && c.param(0).is<Handle<Entity>>())
      c.return_value() = Entity::copy(c.param(0).as<Handle<Entity>>());
  });
  ScriptGlobal("entity_destroy") = (ScriptNativeFunction)([](ScriptContext& c) {
    if(c.param_count() && c.param(0).is<Handle<Entity>>())
      Entity::destroy(c.param(0).as<Handle<Entity>>());
  });
  ScriptGlobal("entity_get") = (ScriptNativeFunction)([](ScriptContext& c) {
    if(c.param_count()) {
      if(NameComponent* name_component = NameComponent::find(c.param(0).get<Symbol>()))
        c.return_value() = name_component->entity();
      else c.return_value() = nullptr;
    } else c.return_value() = nullptr;
  });
  // Material ///////////////////////////////////////////////////////////////////
  Material::script_registration();
  // Devices ///////////////////////////////////////////////////////////////////
  ScriptGlobal("get_devices") = (ScriptNativeFunction)([](ScriptContext& c) {
    auto wtr(ref<Table<Var,Var>>());
    for(Device* device : Device::devices())
      (*wtr)[device->handle()] = true;
    c.return_value() = wtr;
  });
  L_SCRIPT_RETURN_METHOD(Device, "get_axis", 1, axis(Device::symbol_to_axis(c.param(0))));
  L_SCRIPT_RETURN_METHOD(Device, "get_button", 1, button(Device::symbol_to_button(c.param(0))));
  // Script ///////////////////////////////////////////////////////////////////
  L_COMPONENT_BIND(ScriptComponent, "script");
  L_SCRIPT_METHOD(ScriptComponent, "load", 1, load(c.param(0).get<String>()));
  L_SCRIPT_RETURN_METHOD(ScriptComponent, "object", 0, _context.self());
}

void ScriptComponent::load(const char* filename) {
  _script = filename;
  _script.flush();
  start();
}
void ScriptComponent::start() {
  L_ASSERT(_script);
  _started = true;
  _context.execute(ref<ScriptFunction>(*_script));
  _context.try_execute_method(start_symbol);
}
void ScriptComponent::update() {
  _context.try_execute_method(update_symbol);
}
void ScriptComponent::late_update() {
  _context.try_execute_method(late_update_symbol);
}
void ScriptComponent::event(const Ref<Table<Var, Var>>&e) {
  _context.try_execute_method(event_symbol, {e});
}
