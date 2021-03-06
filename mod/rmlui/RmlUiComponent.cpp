#include "RmlUiComponent.h"

#include <L/src/component/Camera.h>

using namespace L;

L::Array<RmlUiComponent::EventListener*> RmlUiComponent::_event_listeners;

static Ref<InputMap> input_map = ref<InputMap>(Array<InputMapEntry>{
  InputMapEntry{"MouseLeft", Device::Button::MouseLeft},
  InputMapEntry{"MouseRight", Device::Button::MouseRight},
  InputMapEntry{"MouseMiddle", Device::Button::MouseMiddle},
});
#if L_RMLUI_WITH_DEBUGGER
static bool debugger_init = false;
#endif

#define RMLUI_SCRIPT_METHOD_INTERNAL(PREFIX, NAME, NARGS, ...) \
  register_rmlui_script_method(NAME, \
    [](ScriptContext& c) { \
      if(c.param_count() < NARGS) { \
        c.warning("Too few arguments in method call '%s.%s'", "RmlUiElement", NAME); \
      } \
      if(Rml::Element* element = rmlui_native_element(c.current_self())) { \
        PREFIX(element->__VA_ARGS__); \
      } else { \
        c.warning("Calling method '%s.%s' on invalid value of type '%s'", "RmlUiElement", NAME, c.current_self().type()->name); \
      } \
    })
#define RMLUI_SCRIPT_METHOD(NAME, NARGS, ...) RMLUI_SCRIPT_METHOD_INTERNAL(, NAME, NARGS, __VA_ARGS__)
#define RMLUI_SCRIPT_RETURN_METHOD(NAME, NARGS, ...) RMLUI_SCRIPT_METHOD_INTERNAL(c.return_value() = rmlui_script_type, NAME, NARGS, __VA_ARGS__)

static RmlUiSafeElement rmlui_script_type(Rml::Element* element) { return element ? element->GetObserverPtr() : nullptr; }
static Ref<Rml::ElementPtr> rmlui_script_type(Rml::ElementPtr element_ptr) { return ref_move<Rml::ElementPtr>(static_cast<Rml::ElementPtr&&>(element_ptr)); }
static String rmlui_script_type(Rml::String string) { return String(string.c_str(), string.size()); }
static bool rmlui_script_type(bool v) { return v; }

static Rml::Element* rmlui_native_element(Var v) {
  if(RmlUiSafeElement* safe_element = v.try_as<RmlUiSafeElement>()) {
    return safe_element->get();
  } else if(Ref<Rml::ElementPtr>* element_ptr_ref = v.try_as<Ref<Rml::ElementPtr>>()) {
    return (*element_ptr_ref)->get();
  }
  return nullptr;
}

static void register_rmlui_script_method(const char* name, ScriptNativeFunction func) {
  register_script_method<RmlUiSafeElement>(name, func);
  register_script_method<Ref<Rml::ElementPtr>>(name, func);
}

void RmlUiComponent::EventListener::ProcessEvent(Rml::Event& event) {
  script_context.self() = event.GetCurrentElement()->GetObserverPtr();
  script_context.execute(function);
}

RmlUiComponent::RmlUiComponent() {
  _context = Rml::CreateContext("main", Rml::Vector2i(128, 128));
  if(_context == nullptr) {
    error("rmlui: Could not create context");
  }

#if L_RMLUI_WITH_DEBUGGER
  if(debugger_init) {
    Rml::Debugger::Shutdown();
  }
  Rml::Debugger::Initialise(_context);
  debugger_init = true;
#endif

  _context->SetDensityIndependentPixelRatio(1.0f);

  _input_context.set_name("rmlui");
  _input_context.set_input_map(input_map);
}

RmlUiComponent::~RmlUiComponent() {
  Rml::RemoveContext(_context->GetName());
}

void RmlUiComponent::update_components() {
  _camera = entity()->require_component<Camera>();
}
void RmlUiComponent::script_registration() {
  L_COMPONENT_BIND(RmlUiComponent, "rmlui");
  L_SCRIPT_RETURN_METHOD(RmlUiComponent, "load_document", 1, load_document(c.param(0).get<String>()));

  RMLUI_SCRIPT_METHOD("show", 0, GetOwnerDocument()->Show());
  RMLUI_SCRIPT_METHOD("hide", 0, GetOwnerDocument()->Hide());
  RMLUI_SCRIPT_METHOD("close", 0, GetOwnerDocument()->Close());

  RMLUI_SCRIPT_RETURN_METHOD("create_element", 0, GetOwnerDocument()->CreateElement(c.param(0).get<String>().begin()));
  RMLUI_SCRIPT_RETURN_METHOD("create_text_node", 0, GetOwnerDocument()->CreateTextNode(c.param(0).get<String>().begin()));

  RMLUI_SCRIPT_RETURN_METHOD("get_address", 0, GetAddress());
  RMLUI_SCRIPT_RETURN_METHOD("get_child", 0, GetChild(int(c.param(0).get<float>())));
  RMLUI_SCRIPT_RETURN_METHOD("get_first_child", 0, GetFirstChild());
  RMLUI_SCRIPT_RETURN_METHOD("get_id", 0, GetId());
  RMLUI_SCRIPT_RETURN_METHOD("get_last_child", 0, GetLastChild());
  RMLUI_SCRIPT_RETURN_METHOD("get_next_sibling", 0, GetNextSibling());
  RMLUI_SCRIPT_RETURN_METHOD("get_num_children", 0, GetNumChildren());
  RMLUI_SCRIPT_RETURN_METHOD("get_parent_node", 0, GetParentNode());
  RMLUI_SCRIPT_RETURN_METHOD("get_previous_sibling", 0, GetPreviousSibling());
  RMLUI_SCRIPT_RETURN_METHOD("get_tag_name", 0, GetTagName());
  RMLUI_SCRIPT_RETURN_METHOD("has_child_nodes", 0, HasChildNodes());
  RMLUI_SCRIPT_RETURN_METHOD("remove_child", 0, RemoveChild(rmlui_native_element(c.param(0))));

  RMLUI_SCRIPT_METHOD("set_attribute", 1, SetAttribute(c.param(0).get<String>().begin(), Rml::String(c.param(1).get<String>().begin())));
  RMLUI_SCRIPT_METHOD("set_class", 1, SetClass(c.param(0).get<String>().begin(), c.param(1)));
  RMLUI_SCRIPT_METHOD("set_id", 1, SetId(c.param(0).get<String>().begin()));
  RMLUI_SCRIPT_METHOD("set_inner_rml", 1, SetInnerRML(c.param(0).get<String>().begin()));

  register_rmlui_script_method("add_event_listener",
    [](ScriptContext& c) {
      if(Rml::Element* element = rmlui_native_element(c.current_self())) {
        EventListener* listener = Memory::new_type<EventListener>();
        listener->element = element->GetObserverPtr();
        listener->function = c.param(1);
        element->AddEventListener(c.param(0).get<String>().begin(), listener);
        _event_listeners.push(listener);
      }
    });

  register_rmlui_script_method("get_element_by_id",
    [](ScriptContext& c) {
      if(Rml::Element* element = rmlui_native_element(c.current_self())) {
        if(Rml::Element* found_element = element->GetElementById(c.param(0).get<String>().begin())) {
          c.return_value() = rmlui_script_type(found_element);
        }
      }
    });

  register_rmlui_script_method("get_elements_by_tag_name",
    [](ScriptContext& c) {
      if(Rml::Element* element = rmlui_native_element(c.current_self())) {
        Rml::ElementList element_list;
        element->GetElementsByTagName(element_list, c.param(0).get<String>().begin());
        Array<Var>& array = c.return_value().make<Ref<Array<Var>>>().make();
        for(Rml::Element* tag_element : element_list) {
          array.push(rmlui_script_type(tag_element));
        }
      }
    });

  register_rmlui_script_method("get_elements_by_class_name",
    [](ScriptContext& c) {
      if(Rml::Element* element = rmlui_native_element(c.current_self())) {
        Rml::ElementList element_list;
        element->GetElementsByClassName(element_list, c.param(0).get<String>().begin());
        Array<Var>& array = c.return_value().make<Ref<Array<Var>>>().make();
        for(Rml::Element* tag_element : element_list) {
          array.push(rmlui_script_type(tag_element));
        }
      }
    });

  register_rmlui_script_method("get_children",
    [](ScriptContext& c) {
      if(Rml::Element* element = rmlui_native_element(c.current_self())) {
        int num_children = element->GetNumChildren();
        Array<Var>& array = c.return_value().make<Ref<Array<Var>>>().make();
        for(int i = 0; i < num_children; i++) {
          array.push(rmlui_script_type(element->GetChild(i)));
        }
      }
    });

  register_rmlui_script_method("append_child",
    [](ScriptContext& c) {
      if(Rml::Element* element = rmlui_native_element(c.current_self())) {
        if(Ref<Rml::ElementPtr>* child_element_ref = c.param(0).try_as<Ref<Rml::ElementPtr>>()) {
          c.return_value() = rmlui_script_type(element->AppendChild(static_cast<Rml::ElementPtr&&>(**child_element_ref)));
        }
      }
    });
}

void RmlUiComponent::update() {
  { // Update dimensions
    const Vector2i viewport_size = _camera->viewport_pixel().size();
    _context->SetDimensions(Rml::Vector2i(viewport_size.x(), viewport_size.y()));
  }

  { // Update input
    const bool ctrl_modifier = _input_context.get_raw_button(Device::Button::LeftCtrl) || _input_context.get_raw_button(Device::Button::RightCtrl);
    const bool shift_modifier = _input_context.get_raw_button(Device::Button::LeftShift) || _input_context.get_raw_button(Device::Button::RightShift);
    const bool alt_modifier = _input_context.get_raw_button(Device::Button::LeftAlt) || _input_context.get_raw_button(Device::Button::RightAlt);
    const int key_modifier_state =
      (ctrl_modifier ? Rml::Input::KeyModifier::KM_CTRL : 0) |
      (shift_modifier ? Rml::Input::KeyModifier::KM_SHIFT : 0) |
      (alt_modifier ? Rml::Input::KeyModifier::KM_ALT : 0);

    // Mouse move
    _context->ProcessMouseMove(Window::cursor_x(), Window::cursor_y(), key_modifier_state);

    // Block mouse input if outstanding element is hovered
    Rml::Element* hover_el = _context->GetHoverElement();
    _input_context.set_block_mode(hover_el && hover_el->GetTagName() != "body" ? InputBlockMode::Used : InputBlockMode::None);

#if L_RMLUI_WITH_DEBUGGER
    // Toggle debugger
    if(ctrl_modifier && _input_context.get_raw_button_pressed(Device::Button::F8)) {
      Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
    }
#endif

    { // Mouse buttons
      Device::Button mouse_buttons[] = {
        Device::Button::MouseLeft,
        Device::Button::MouseRight,
        Device::Button::MouseMiddle,
      };
      for(int i = 0; i < 3; i++) {
        if(_input_context.get_raw_button_pressed(mouse_buttons[i])) {
          _context->ProcessMouseButtonDown(i, key_modifier_state);
        }
        if(_input_context.get_raw_button_released(mouse_buttons[i])) {
          _context->ProcessMouseButtonUp(i, key_modifier_state);
        }
      }
    }
  }

  _context->Update();
  _context->Render();
}

RmlUiSafeElement RmlUiComponent::load_document(const char* path) {
  if(Rml::ElementDocument* document = _context->LoadDocument(path)) {
    return document->GetObserverPtr();
  }
  return RmlUiSafeElement();
}

#if !L_RLS
void RmlUiComponent::refresh() {
  const int num_docs = _context->GetNumDocuments();
  for(int i = 0; i < num_docs; i++) {
    Rml::ElementDocument* document = _context->GetDocument(i);
    if(!document->GetSourceURL().empty()) {
      document->ReloadStyleSheet();
    }
  }
}
#endif
