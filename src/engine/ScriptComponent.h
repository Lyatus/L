#pragma once

#include "Component.h"
#include "../script/Context.h"
#include "../system/Device.h"

namespace L {
  class ScriptComponent : public Component {
    L_COMPONENT(ScriptComponent)
  protected:
    Script::Context _context;
  public:
    void updateComponents();
    void load(const char* filename);
    void update();
    void lateUpdate();
    void event(const Device::Event&);
    void event(const Window::Event&);
    void event(const Ref<Table<Var, Var>>&);
    void gui(const Camera&);
    static void init();
  };
}
