#include "00Names.hpp"
#include "IconsFontAwesome6.h"
#include <fstream>

Project project;

int main() {
  MvWindow window("OreMapper");
  MvFont font({{getProgramPath().parent_path() / "Assets/FontAwesome.ttf", {{ICON_MIN_FA, ICON_MAX_FA}}}, {getProgramPath().parent_path() / "Assets/OpenSans-Regular.ttf", {{' ', '~'}}}}, 24);
  window.setFont(font);

  project.load("/mnt/Dev/Arduino/Projects/GameBoyStory/res/Mario/Project"); // TODO: last project

  MvGui::setWindow(window);

  { // TODO: Default layout
    auto& dockspace = MvGui::getDockspace();
    auto& bottom = dockspace.split(MvGuiDockDirection::Bottom, 0.3);
    bottom.split(MvGuiDockDirection::Left, 0.7).dock("Assets");
    bottom.left().dock("Atlas");
    dockspace.left().split(MvGuiDockDirection::Right, 0.2).dock("Inspector");
    dockspace.left().left().dock("Mapper");
  }

  while (window.isOpen()) {
    if (Mova::isKeyPressed(MvKey::F5)) project.load(project.path);                                                  // * Reload
    if (Mova::isKeyHeld(MvKey::Ctrl) && Mova::isKeyPressed(MvKey::E)) project.exportAll(project.path / "data.raw"); // * Export

    window.clear(MvColor(115, 140, 152));
    MvGui::newFrame();

    Editor::windows();

    MvGui::endFrame();
    Mova::nextFrame();
  }
  return 0;
}
