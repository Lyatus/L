#pragma once

#include "../container/Array.h"
#include "../container/Handle.h"
#include "../container/Ref.h"
#include "../container/Table.h"
#include "../input/Device.h"
#include "../text/Symbol.h"

namespace L {
  struct InputMapEntry {
    Symbol name;
    union {
      Device::Button button;
      Device::Axis axis;
    };
    bool is_button = true;
    float offset = 0.f;
    float multiplier = 1.f;
  };

  typedef Array<InputMapEntry> InputMap;

  enum class InputBlockMode : uint8_t {
    None,
    Used,
    All,
  };

  class InputContext : public Handled<InputContext> {
  protected:
    typedef Bitfield<size_t(Device::Button::Last) + size_t(Device::Axis::Last)> InputMask;
    static Array<Handle<InputContext>> _contexts;
    Ref<InputMap> _input_map;
    Handle<Device> _device;
    InputBlockMode _block_mode = InputBlockMode::Used;

    Bitfield<size_t(Device::Button::Last)> _buttons;
    float _axes[size_t(Device::Axis::Last)];
    Table<Symbol, float> _inputs;

    Bitfield<size_t(Device::Button::Last)> _previous_buttons;
    float _previous_axes[size_t(Device::Axis::Last)];
    Table<Symbol, float> _previous_inputs;

  public:
    InputContext();

    void set_input_map(const Ref<InputMap>&);
    void set_block_mode(InputBlockMode);

    bool get_raw_button(Device::Button) const;
    bool get_raw_button_pressed(Device::Button) const;
    float get_raw_axis(Device::Axis) const;

    bool get_button(const Symbol&) const;
    bool get_button_pressed(const Symbol&) const;
    float get_axis(const Symbol&) const;

    static void update();
    static void script_registration();
  };
}
