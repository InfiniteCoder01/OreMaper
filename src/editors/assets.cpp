#include "../00Names.hpp"
#include "../IconsFontAwesome6.h"

namespace Editor::Assets {
static std::filesystem::path directory;
static std::filesystem::path removeTarget;

void reset() {
  directory = "";
  removeTarget = "";
}

bool fileEntry(const std::filesystem::directory_entry& entry) {
  MvGui::pushStyle(MvGui::getStyle().widgetRounding, 0);

  const bool clicked = MvGui::Button(entry.path().filename().string());
  MvGui::sameLine();

  MvGui::pushStyle(MvGui::getStyle().foregroundColor, MvColor::red);
  if (std::filesystem::equivalent(entry.path(), removeTarget)) {
    if (MvGui::Button("Remove")) {
      std::filesystem::remove(removeTarget);
      removeTarget = "";
    }
    MvGui::popStyle(MvGui::getStyle().foregroundColor);
    MvGui::sameLine();
    if (MvGui::Button("Cancel")) removeTarget = "";
  } else {
    if (MvGui::Button(ICON_FA_TRASH_CAN)) removeTarget = entry.path();
    MvGui::popStyle(MvGui::getStyle().foregroundColor);
  }

  MvGui::popStyle(MvGui::getStyle().widgetRounding);
  return clicked;
}

void window() {
  if (!directory.empty() && !std::filesystem::exists(project.path / directory)) directory = "";
  if (!removeTarget.empty() && !std::filesystem::exists(removeTarget)) removeTarget = "";

  MvGui::Begin("Assets");
  MvGui::TextUnformatted(directory.string());
  if (directory != "" && MvGui::Button("..")) directory = directory.parent_path();
  for (const auto& entry : std::filesystem::directory_iterator(project.path / directory)) {
    if (entry.path().extension() == ".atl") continue;
    if (fileEntry(entry)) {
      if (entry.is_directory()) directory = entry.path().lexically_relative(project.path);
      else {
        if (entry.path().extension() == ".png") {
          if (Inspector::atlas) {
            *Inspector::atlas = entry.path().lexically_relative(project.path).string();
            Inspector::atlas = nullptr;
          } else AtlasView::open(entry.path().lexically_relative(project.path));
        } else if (entry.path().extension() == ".map") {
          if (Inspector::map) {
            *Inspector::map = entry.path().lexically_relative(project.path).string();
            Inspector::map = nullptr;
          } else Mapper::open(entry.path().lexically_relative(project.path));
        } else if (entry.path().extension() == ".cmp") {
          if (!Inspector::addingComponent) Inspector::reset();
          Inspector::component = entry.path().lexically_relative(project.path);
        }
      }
    }
  }
  MvGui::End();
}
} // namespace Editor::Assets
