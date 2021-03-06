#include <L/src/engine/Engine.h>
#include <L/src/engine/Resource.h>
#include <L/src/math/math.h>
#include <L/src/rendering/Color.h>
#include <L/src/text/format.h>
#include <imgui_integration.h>

#include <L/src/engine/Resource.inl>

using namespace L;

#if !L_RLS

static bool opened = false;

static Table<Symbol, Color> type_colors;
static uint32_t state_mask = (1 << uint32_t(ResourceState::Loading)) | (1 << uint32_t(ResourceState::Failed));

static void imgui_resources_update() {
  if(imgui_begin_main_menu_bar()) {
    if(ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("Resources", "", &opened);
      ImGui::EndMenu();
    }
    imgui_end_main_menu_bar();
  }

  if(!opened) {
    return;
  }

  if(!ImGui::Begin("Resources", &opened)) {
    ImGui::End();
    return;
  }

  Array<ResourceSlot*> slots = ResourceSlot::slots();

  if(ImGui::BeginPopup("State Filter")) {
    ImGui::CheckboxFlags("Unloaded", &state_mask, 1 << uint32_t(ResourceState::Unloaded));
    ImGui::CheckboxFlags("Loading", &state_mask, 1 << uint32_t(ResourceState::Loading));
    ImGui::CheckboxFlags("Loaded", &state_mask, 1 << uint32_t(ResourceState::Loaded));
    ImGui::CheckboxFlags("Failed", &state_mask, 1 << uint32_t(ResourceState::Failed));
    ImGui::EndPopup();
  }

  ImGui::Columns(5, "Columns");

  ImGui::SetColumnWidth(0, 128);
  ImGui::SetColumnWidth(1, max(16.f, ImGui::GetWindowContentRegionWidth() - 128 - 64 - 64 - 64));
  ImGui::SetColumnWidth(2, 64);
  ImGui::SetColumnWidth(3, 64);
  ImGui::SetColumnWidth(4, 64);

  ImGui::Separator();

  ImGui::Text("Type");
  ImGui::NextColumn();

  ImGui::Text("Id");
  ImGui::NextColumn();

  ImGui::Text("CPU");
  ImGui::NextColumn();

  ImGui::Text("GPU");
  ImGui::NextColumn();

  if(ImGui::Button("State")) {
    ImGui::OpenPopup("State Filter");
  }
  ImGui::NextColumn();

  ImGui::Separator();

  for(ResourceSlot* slot : slots) {
    if(((1 << uint32_t(slot->state.load())) & state_mask) == 0) {
      continue;
    }

    bool type_color_created;
    Color* type_color = type_colors.find_or_create(slot->type, &type_color_created);
    if(type_color_created) {
      *type_color = Color::from_index(type_colors.count() - 1);
    }
    const ImColor im_type_color = ImColor(type_color->rf(), type_color->gf(), type_color->bf());
    ImColor state_color = ImColor(1.f, 1.f, 1.f);
    const char* state = "Unknown";
    switch(slot->state) {
      case ResourceState::Unloaded:
        state = "Unloaded";
        state_color = ImColor(0.5f, 0.5f, 0.5f);
        break;
      case ResourceState::Loading:
        state = "Loading";
        state_color = ImColor(1.f, 0.5f, 0.f);
        break;
      case ResourceState::Loaded:
        state = "Loaded";
        state_color = ImColor(0.f, 1.f, 0.f);
        break;
      case ResourceState::Failed:
        state = "Failed";
        state_color = ImColor(1.f, 0.f, 0.f);
        break;
    }

    String type = String(slot->type).replace_all("L::", "");
    ImGui::TextColored(im_type_color, "%s", type.begin());
    ImGui::NextColumn();

    ImGui::Text("%s", (const char*)slot->id);
    ImGui::NextColumn();

    if(slot->cpu_size > 0) {
      String cpu_size = format_memory_amount(slot->cpu_size);
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(cpu_size.begin()).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
      ImGui::Text("%s", cpu_size.begin());
    }
    ImGui::NextColumn();

    if(slot->gpu_size > 0) {
      String gpu_size = format_memory_amount(slot->gpu_size);
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(gpu_size.begin()).x - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
      ImGui::Text("%s", format_memory_amount(slot->gpu_size).begin());
    }
    ImGui::NextColumn();

    ImGui::TextColored(state_color, "%s", state);
    ImGui::NextColumn();
  }
  ImGui::Columns(1);

  ImGui::End();
}
#endif

void imgui_resources_module_init() {
#if !L_RLS
  Engine::add_update(imgui_resources_update);
#endif
}
