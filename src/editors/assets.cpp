#include "../00Names.hpp"
#include "../IconsFontAwesome6.h"
#include <fstream>

namespace Editor::Assets {
static std::filesystem::path directory;
static std::filesystem::path moveTarget, renameTarget, removeTarget;

void reset() {
  directory = "";
  moveTarget = renameTarget = removeTarget = "";
}

static void replaceProperty(std::filesystem::path path, std::filesystem::path newPath) {
  for (const auto& entry : std::filesystem::recursive_directory_iterator(project.path)) {
    if (entry.is_directory() || entry.path().extension() != ".map") continue;
    auto& map = project.map(entry.path().lexically_relative(project.path));
    if (map.atlas == path) map.atlas = newPath;
    for (auto& object : map.objects) {
      for (auto& component : object.components) {
        if (component.path == path) component.path = newPath;
        for (auto& property : component.properties) {
          if (property == path.string()) property = newPath.string();
        }
      }
    }
    project.closeMap(entry.path().lexically_relative(project.path));
  }
}

static void moveFile(std::filesystem::path path, std::filesystem::path newPath) {
  if (path.extension() == ".png") {
    replaceProperty(path, newPath);

    auto& atlas = project.atlas(path);
    std::filesystem::rename(project.path / path, project.path / newPath);
    const auto original = path;
    path.replace_extension(".atl");
    newPath.replace_extension(".atl");
    atlas.path = project.path / newPath;
    project.closeAtlas(original);
  }

  if (path.extension() == ".cmp") {
    replaceProperty(path, newPath);

    auto& component = project.component(path);
    component.path = project.path / newPath;
    project.closeComponent(path);
  }

  if (path.extension() == ".map") {
    replaceProperty(path, newPath);
    if (Mapper::map == path) Mapper::map = newPath;

    auto& map = project.map(path);
    map.path = project.path / newPath;
    project.closeMap(path);
  }
  std::filesystem::remove(project.path / path);
}

static bool fileEntry(const std::filesystem::directory_entry& entry) {
  MvGui::pushStyle(MvGui::getStyle().widgetRounding, 0);

  const bool clicked = MvGui::Button(entry.path().filename().string());

  MvGui::sameLine();
  if (MvGui::Button(ICON_FA_FILE_ARROW_UP)) moveTarget = entry.path();

  static std::string newName;
  if (std::filesystem::equivalent(entry.path(), renameTarget)) {
    static MvGuiTextInputState state;
    MvGui::sameLine();
    MvGui::TextInput(newName, state);
    MvGui::sameLine();
    const bool ok = MvGui::Button(ICON_FA_CHECK) || (state.cursor != UINT32_MAX && Mova::isKeyPressed(MvKey::Enter));
    MvGui::sameLine();
    const bool cancel = MvGui::Button(ICON_FA_XMARK) || (state.cursor != UINT32_MAX && Mova::isKeyPressed(MvKey::Escape));
    if (ok) {
      moveFile(entry.path(), entry.path().parent_path() / (newName + entry.path().extension().string()));
      renameTarget = newName = "";
      return false;
    }
    if (cancel) renameTarget = newName = "";
  } else {
    MvGui::sameLine();
    if (MvGui::Button(ICON_FA_PENCIL)) renameTarget = entry.path(), newName = entry.path().stem();
  }

  MvGui::sameLine();
  MvGui::pushStyle(MvGui::getStyle().foregroundColor, MvColor::red);
  if (std::filesystem::equivalent(entry.path(), removeTarget)) {
    if (MvGui::Button("Remove")) {
      if (removeTarget.extension() == ".png") project.closeAtlas(removeTarget.lexically_relative(project.path));
      if (removeTarget.extension() == ".map") project.closeMap(removeTarget.lexically_relative(project.path));
      if (removeTarget.extension() == ".cmp") project.closeComponent(removeTarget.lexically_relative(project.path));
      std::filesystem::remove(removeTarget);
      if (removeTarget.extension() == ".png") {
        removeTarget.replace_extension(".atl");
        std::filesystem::remove(removeTarget);
      }
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

  static bool creatingComponent = false, creatingMap = false, creatingDirectory = false;
  if (creatingComponent || creatingMap || creatingDirectory) {
    static std::string filename;
    static MvGuiTextInputState state;
    MvGui::TextInput(filename, state);
    MvGui::sameLine();
    const bool ok = MvGui::Button(ICON_FA_CHECK) || (state.cursor != UINT32_MAX && Mova::isKeyPressed(MvKey::Enter));
    MvGui::sameLine();
    const bool cancel = MvGui::Button(ICON_FA_XMARK) || (state.cursor != UINT32_MAX && Mova::isKeyPressed(MvKey::Escape));
    if (ok) {
      if (creatingComponent) Component(project.path / directory / (filename + ".cmp"), {});
      else if (creatingMap) Map(10, 10, AtlasView::atlas, project.path / directory / (filename + ".map"));
      else if (creatingDirectory) std::filesystem::create_directories(project.path / directory / filename);
    }
    if (ok || cancel) {
      filename = "";
      creatingComponent = creatingMap = creatingDirectory = false;
    }
  } else {
    if (!moveTarget.empty()) {
      if (MvGui::Button(ICON_FA_PASTE)) {
        if (std::filesystem::is_directory(moveTarget)) {
          for (const auto& entry : std::filesystem::recursive_directory_iterator(moveTarget)) {
            if (entry.is_directory()) continue;
            std::filesystem::create_directories(entry.path().parent_path());
            auto projectRelative = entry.path().lexically_relative(project.path);
            moveFile(projectRelative, directory / moveTarget.filename() / projectRelative.lexically_relative(moveTarget.lexically_relative(project.path)));
          }
        } else moveFile(moveTarget.lexically_relative(project.path), directory / moveTarget.filename());
        moveTarget = "";
      }
      MvGui::sameLine();
    } else {

      if (MvGui::Button(ICON_FA_PUZZLE_PIECE)) creatingComponent = true;
      MvGui::sameLine();
      if (!AtlasView::atlas.empty() && MvGui::Button(ICON_FA_MAP)) creatingMap = true;
      MvGui::sameLine();
    }
    if (MvGui::Button("\uf65e")) creatingDirectory = true;
  }
  MvGui::End();
}
} // namespace Editor::Assets
