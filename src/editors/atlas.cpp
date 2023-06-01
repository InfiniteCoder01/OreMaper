#include "../00Names.hpp"

namespace Editor::AtlasView {
std::filesystem::path atlas;
Rect<uint32_t> selection;
inline Atlas& currentAtlas() { return project.atlas(atlas); }

void reset() {
  if (!atlas.empty()) project.closeAtlas(atlas);
  atlas = "";
  selection = Rect<uint32_t>(0, 1);
}

void window() {
  MvGui::Begin("Atlas");
  if (atlas.empty()) {
    reset();
    MvGui::TextUnformatted("No atlas is opened!");
    MvGui::End();
    return;
  }
  if (!std::filesystem::exists(project.path / atlas)) {
    atlas = "";
    reset();
    MvGui::End();
    return;
  }

  { // * Tilesize input
    static MvGuiTextInputState widthState, heightState;
    static std::string width, height;

    if (widthState.cursor == UINT32_MAX) width = std::to_string(currentAtlas().tileSize.x);
    if (heightState.cursor == UINT32_MAX) height = std::to_string(currentAtlas().tileSize.y);

    MvGui::TextInput(width, widthState, MvGuiTextInputType::Integer);
    MvGui::sameLine();
    MvGui::TextUnformatted("x");
    MvGui::sameLine();
    MvGui::TextInput(height, heightState, MvGuiTextInputType::Integer);

    if (widthState.cursor == UINT32_MAX) currentAtlas().tileSize.x = Math::clamp(std::stoi(width), 1, currentAtlas()().width());
    if (heightState.cursor == UINT32_MAX) currentAtlas().tileSize.y = Math::clamp(std::stoi(height), 1, currentAtlas()().height());
  }

  auto& image = MvGui::getTarget();

  // * Calculate Size
  const auto scale = min(MvGui::getContentRegionAvailable().x / (float)currentAtlas()().width(), MvGui::getContentRegionAvailable().y / (float)currentAtlas()().height());
  const auto atlasSize = vec2f(currentAtlas()().size()) * scale;
  MvGui::widget(atlasSize);

  // * Update
  auto tileSize = vec2f(currentAtlas().tileSize) * scale;
  if (MvGui::isWidgetHovered()) {
    const vec2u hoveredTile = vec2f(MvGui::getWindow().getMousePosition() - MvGui::getWidgetRect().position()) / tileSize;
    { // Selection
      static vec2u selectionStart;
      if (Mova::isMouseButtonPressed(MvMouseLeft)) selectionStart = hoveredTile;
      if (Mova::isMouseButtonHeld(MvMouseLeft)) {
        vec2i size = hoveredTile - selectionStart + 1;
        if (size.x <= 0) size.x -= 1;
        if (size.y <= 0) size.y -= 1;
        selection = Rect<uint32_t>(selectionStart, size);
      }
    }
  }

  // * Draw
  image.drawImage(currentAtlas()(), MvGui::getWidgetRectViewportRelative().position(), atlasSize);
  image.drawRect(vec2f(selection.position()) * tileSize + MvGui::getWidgetRectViewportRelative().position(), vec2f(selection.size()) * tileSize, MvColor::red, 5);

  MvGui::End();
}

void exportAtlases(FILE* file) {
  // std::string data;
  // {
  //   project.atlasIndex.clear();

  //   uint32_t nAtlases = 0;
  //   for (const auto& entry : std::filesystem::recursive_directory_iterator(project.path)) {
  //     if (entry.path().extension() != ".png") continue;
  //     project.atlasIndex[entry.path().lexically_relative(project.path)] = nAtlases;
  //     nAtlases++;
  //   }
  //   uintToBytes(data, nAtlases, 1);
  // }
  // for (const auto& entry : std::filesystem::recursive_directory_iterator(project.path)) {
  //   if (entry.path().extension() != ".png") continue;

  //   const auto& atlas = project.atlas(entry.path().lexically_relative(project.path));

  //   uintToBytes(data, atlas.tileSize.x, 1);
  //   uintToBytes(data, atlas.tileSize.y, 1);
  //   uintToBytes(data, atlas.texture.width() / atlas.tileSize.x * atlas.texture.height() / atlas.tileSize.y, 1);

  //   for (uint32_t y = 0; y < atlas.texture.height(); y += atlas.tileSize.y) {
  //     for (uint32_t x = 0; x < atlas.texture.width(); x += atlas.tileSize.x) {
  //       for (uint32_t y1 = 0; y1 < atlas.tileSize.y; y1++) {
  //         for (uint32_t x1 = 0; x1 < atlas.tileSize.x; x1++) {
  //           const auto pixel = atlas.texture.getPixel(x + x1, y + y1);
  //           uint16_t color = 0;
  //           if (pixel.a < 128) color = 0xF81F;
  //           else {
  //             color = ((pixel.r & 0xF8) << 8);
  //             color |= ((pixel.g & 0xFC) << 3);
  //             color |= (pixel.b >> 3);
  //           }
  //           uintToBytes(data, color >> 8, 1);
  //           uintToBytes(data, color, 1);
  //         }
  //       }
  //     }
  //   }
  //   data += "\n  ";

  //   project.closeAtlas(entry.path().lexically_relative(project.path));
  // }
  // return data;
}
} // namespace Editor::AtlasView
