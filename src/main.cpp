#include "audio/AudioEngine.h"
#include "engine/debug_draw.h"
#include "engine/Engine.h"
#include "engine/Resource.inl"
#include "engine/Settings.h"
#include "network/Network.h"
#include "system/Arguments.h"
#include "system/File.h"

#include "component/AudioListenerComponent.h"
#include "component/AudioSourceComponent.h"
#include "component/Camera.h"
#include "component/Collider.h"
#include "component/GUIComponent.h"
#include "component/GroupComponent.h"
#include "component/HierarchyComponent.h"
#include "component/InputComponent.h"
#include "component/MidiSourceComponent.h"
#include "component/NameComponent.h"
#include "component/PostProcessComponent.h"
#include "component/Primitive.h"
#include "component/RigidBody.h"
#include "component/ScriptComponent.h"
#include "component/SkeletalAnimatorComponent.h"
#include "component/Transform.h"

using namespace L;

void mainjob(void*) {
  Network::init();

  while(Window::opened()) {
    Engine::update();
  }

  log("exit: clear engine");
  Engine::clear();

  log("exit: clear material cache");
  Material::clear_cache();

  log("exit: shutdown engine");
  Engine::shutdown();

  log("exit: join all tasks");
  TaskSystem::join_all();

#if L_PROFILING
  log("exit: flush profiling");
  flush_profiling();
#endif
}
int main(int argc, const char* argv[]) {
  Arguments::init(argc, argv);

#if !L_RLS
  init_log_file();
  init_debug_draw();
  Engine::add_parallel_update(ResourceSlot::update);
  ResourceLoading<Buffer>::add_loader(
    [](ResourceSlot& slot, Buffer::Intermediate& intermediate) {
      intermediate = slot.read_source_file();
      return true;
    });
#endif

  Date program_mtime;
  if(argc > 0 && File::mtime(argv[0], program_mtime)) {
    ResourceSlot::set_program_mtime(program_mtime);
  }

  init_type();
  init_script_standard_functions();

  Engine::register_component<Transform>();
  Engine::register_component<Camera>();
  Engine::register_component<RigidBody>();
  Engine::register_component<Collider>();
  Engine::register_component<InputComponent>();
  Engine::register_component<ScriptComponent>();
  Engine::register_component<AudioSourceComponent>();
  Engine::register_component<AudioListenerComponent>();
  Engine::register_component<MidiSourceComponent>();
  Engine::register_component<NameComponent>();
  Engine::register_component<GroupComponent>();
  Engine::register_component<HierarchyComponent>();
  Engine::register_component<PostProcessComponent>();
  Engine::register_component<Primitive>();
  Engine::register_component<GUIComponent>();
  Engine::register_component<SkeletalAnimatorComponent>();

  {
    L_SCOPE_MARKER("Initializing modules");
#define MOD_USED_1(M) { \
      log("Initializing module: %s", #M); \
      L_SCOPE_MARKER(#M); \
      extern void M##_module_init(); \
      M##_module_init(); \
    }
#define MOD_USED_0(M)
#define MOD(M) L_CONCAT(MOD_USED_, L_USE_MODULE_##M)(M)
#include "mod_list.gen" // Generated by CMakeLists.txt
  }

  Engine::init();
  AudioEngine::init_instance();

  const char* window_name(Settings::get_string("window_name", "L Engine Sample"));

  uint32_t window_flags = 0;
  window_flags |= Settings::get_int("no_cursor", 0) ? Window::nocursor : 0;
  window_flags |= Settings::get_int("resizable_window", 0) ? Window::resizable : 0;

  if(Settings::get_int("fullscreen", 1))
    Window::instance()->open_fullscreen(window_name, window_flags);
  else
    Window::instance()->open(window_name, Settings::get_int("resolution_x", 1024), Settings::get_int("resolution_y", 768), window_flags);

  TaskSystem::push(mainjob, nullptr, uint32_t(-1), TaskSystem::MainTask);
  TaskSystem::init();
  return 0;
}

#if L_RLS && L_WINDOWS
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  main(0, nullptr);
}
#endif
