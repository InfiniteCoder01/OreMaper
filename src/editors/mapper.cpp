#include "../00Names.hpp"

namespace Editor::Mapper {
Map& currentMap() { return project.map(map); }
std::filesystem::path map;
Object* currentObject;

float scale;
vec2i camera;

void reset() {
  if (!map.empty()) project.closeMap(map);
  map = "";
  currentObject = nullptr;
  scale = 1.f;
  camera = 0;
}

void window() {
  MvGui::Begin("Mapper");
  if (map.empty()) {
    reset();
    MvGui::TextUnformatted("No map is opened!");
    MvGui::End();
    return;
  }

  if (!std::filesystem::exists(project.path / map)) {
    map = "";
    reset();
    MvGui::End();
    return;
  }

  // * Drawment
  auto& image = MvGui::getTarget();
  MvGui::widget(MvGui::getContentRegionAvailable() - vec2i(0, image.getFont().height() + MvGui::getStyle().framePadding.y * 2 + MvGui::getStyle().contentPadding.y));
  auto oldViewport = image.getViewport();
  image.setViewport(MvGui::getWidgetRectTargetRelative());

  // * Panning and Zooming
  vec2i mouse = vec2i(Mova::getMousePosition()) - MvGui::getWidgetRect().position();
  if (MvGui::isWidgetHovered()) {
    if (Mova::isMouseButtonHeld(MvMouseMiddle)) camera -= Mova::getMouseDelta();
    if (Mova::getScrollY() != 0) {
      auto lastScale = scale;
      scale *= pow(10, Mova::getScrollY() * -0.1f);
      camera = (mouse + camera) * scale / lastScale - mouse;
    }
  }
  mouse = (mouse + camera) / scale;
  const vec2u screenTileSize = currentMap().getAtlas().tileSize * scale;

  // * Mouse Interactions
  vec2i tileUnderMouse = -1;
  Object* objectUnderMouse = nullptr;
  if (MvGui::isWidgetHovered()) {
    tileUnderMouse = mouse / currentMap().getAtlas().tileSize;
    for (auto& object : currentMap().objects) {
      auto* atlas = object.getAtlas();
      bool hovered = false;
      if (atlas) hovered = Rect<int32_t>(object.position, atlas->tileSize).contains(mouse);
      else hovered = Rect<int32_t>(object.position - 1, 2).contains(mouse);
      if (hovered) {
        objectUnderMouse = &object;
        break;
      }
    }

    if (Mova::isMouseButtonPressed(MvMouseLeft) && Mova::isKeyHeld(MvKey::Shift)) { // * Add Object
      currentMap().objects.emplace_back(mouse);
      if (!AtlasView::atlas.empty()) {
        currentMap().objects.rbegin()->components.push_back(IComponent("builtin/atlas", {AtlasView::atlas}));
      }
      Inspector::reset();
      currentObject = &*currentMap().objects.rbegin();
    } else if (Mova::isMouseButtonHeld(MvMouseLeft) && !Mova::isKeyHeld(MvKey::Shift)) {
      if (objectUnderMouse) {
        Inspector::reset();
        currentObject = objectUnderMouse;                                          // * Select Object
      } else if (AtlasView::atlas == currentMap().atlas && tileUnderMouse != -1) { // * Set Tile
        for (uint32_t x = 0; x < AtlasView::selection.width; x++) {
          for (uint32_t y = 0; y < AtlasView::selection.height; y++) {
            currentMap().set(tileUnderMouse + vec2u(x, y), AtlasView::selection.position() + vec2u(x, y));
          }
        }
      }
    } else if (Mova::isMouseButtonHeld(MvMouseRight)) {
      if (objectUnderMouse) { // * Remove Object
        currentMap().objects.erase(std::remove_if(currentMap().objects.begin(), currentMap().objects.end(),
                                       [&objectUnderMouse](Object& object) {
                                         return &object == objectUnderMouse;
                                       }),
            currentMap().objects.end());
      } else if (tileUnderMouse != -1) currentMap().set(tileUnderMouse, -1); // * Remove Tile
    }
  }

  // * Object interactions
  if (currentObject) {
    currentObject->position.x += Mova::isKeyRepeated(MvKey::Right) - Mova::isKeyRepeated(MvKey::Left);
    currentObject->position.y += Mova::isKeyRepeated(MvKey::Down) - Mova::isKeyRepeated(MvKey::Up);
  }

  // * Draw tiles
  image.fillRect(-camera, currentMap().size * screenTileSize, MvColor(61, 167, 233));
  for (uint32_t y = max(camera.y / int32_t(screenTileSize.y), 0); y < min((camera.y + image.height()) / screenTileSize.y + 1, int32_t(currentMap().size.y)); y++) {
    for (uint32_t x = max(camera.x / int32_t(screenTileSize.x), 0); x < min((camera.x + image.width()) / screenTileSize.x + 1, int32_t(currentMap().size.x)); x++) {
      auto tile = currentMap()(x, y);
      auto tileSize = currentMap().getAtlas().tileSize;
      if (tile != -1) {
        image.drawImage(currentMap().getAtlas()(), vec2u(x, y) * screenTileSize - camera, max(screenTileSize, 1), tile * tileSize, tileSize);
      }
    }
  }

  // * Draw Objects
  for (auto& object : currentMap().objects) {
    auto* atlas = object.getAtlas();
    if (atlas) {
      image.drawImage((*atlas)(), object.position * scale - camera, atlas->tileSize * scale, 0, atlas->tileSize);
      if (&object == currentObject) image.drawRect(object.position * scale - camera, atlas->tileSize * scale, MvColor::red);
    } else image.fillRect(object.position * scale - camera - scale, scale * 3, &object == currentObject ? MvColor::red : MvColor::blue);
  }

  // * Draw Highlighted Tile
  if (tileUnderMouse != -1) {
    image.drawRect(tileUnderMouse * screenTileSize - camera, AtlasView::selection.size() * screenTileSize, MvColor::red);
    image.setViewport(oldViewport);
    auto tile = currentMap()(tileUnderMouse);
    const std::string tileString = tile >= 0 ? std::to_string(tile.x + tile.y * currentMap().getAtlas().texture.width() / currentMap().getAtlas().tileSize.x) : "air";
    MvGui::Text("Tile at %d, %d is %s", tileUnderMouse.x, tileUnderMouse.y, tileString.c_str());
  }

  MvGui::End();
}

std::string exportMaps() {
  std::string data;
  {
    uint32_t nMaps = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(project.path)) {
      if (entry.path().extension() != ".map") continue;
      project.mapIndex[entry.path().lexically_relative(project.path)] = nMaps;
      nMaps++;
    }
    uintToBytes(data, nMaps, 1);
  }
  for (const auto& entry : std::filesystem::recursive_directory_iterator(project.path)) {
    if (entry.path().extension() != ".map") continue;

    auto& map = project.map(entry.path().lexically_relative(project.path));
    const auto& atlas = project.atlas(map.atlas);

    uintToBytes(data, map.size.x, 2);
    uintToBytes(data, map.size.y, 2);

    for (uint32_t y = 0; y < map.size.y; y++) {
      for (uint32_t x = 0; x < map.size.x; x++) {
        const auto tile = map(x, y);
        if (tile == -1) uintToBytes(data, 0, 1);
        else uintToBytes(data, tile.x + tile.y * (atlas.texture.width() / atlas.tileSize.x) + 1, 1);
      }
    }

    uintToBytes(data, map.objects.size(), 2);
    for (const auto& object : map.objects) {
      std::string componentData;
      uintToBytes(componentData, object.position.x, 2);
      uintToBytes(componentData, object.position.y, 2);
      uintToBytes(componentData, object.components.size(), 1);
      for (const auto& component : object.components) {
        uintToBytes(componentData, project.componentIndex[component.path], 2);
        for (uint32_t i = 0; i < component.properties.size(); i++) {
          const auto& parent = project.component(component.path).properties[i];
          const auto& property = component.properties[i];
          if (parent.type == PropertyType::Int) uintToBytes(componentData, std::stoi(property), 4);
          else if (parent.type == PropertyType::String) throw std::runtime_error("String properties are not implemented yet."); // TODO: !
          else if (parent.type == PropertyType::Map) uintToBytes(componentData, project.mapIndex[property], 1);
          else if (parent.type == PropertyType::Atlas) uintToBytes(componentData, project.atlasIndex[property], 1);
        }
      }
      uintToBytes(data, std::count(componentData.begin(), componentData.end(), ','), 2);
      data += componentData;
    }

    data.pop_back();
    data += "\n  ";

    project.closeMap(entry.path().lexically_relative(project.path));
  }
  return data;
}
} // namespace Editor::Mapper

#pragma region MapFunctions
Atlas& Map::getAtlas() const { return project.atlas(atlas); }
Map::Map(std::filesystem::path path) : m_Path(std::move(path)) {
  FILE* file = fopen(m_Path.string().c_str(), "rb");
  fread(&size, sizeof(size), 1, file);
  m_Data.resize(size.x * size.y);
  atlas = freadstr(file);
  fread(m_Data.data(), sizeof(m_Data[0]), m_Data.size(), file);
  objects.resize(fread<uint32_t>(file));
  for (auto& object : objects) {
    const uint32_t x = fread<int32_t>(file), y = fread<int32_t>(file);
    object.position = vec2i(x, y);
    object.components.resize(fread<uint32_t>(file));
    for (auto& component : object.components) {
      component.path = freadstr(file);
      auto& componentClass = project.component(component.path);
      component.properties.resize(componentClass.properties.size());
      for (auto& property : component.properties) property = freadstr(file);
    }
  }
  fclose(file);
}

Map::~Map() {
  Editor::Inspector::reset();

  FILE* file = fopen(m_Path.string().c_str(), "wb");
  fwrite(&size, sizeof(size), 1, file);
  fwritestr(file, atlas);
  fwrite(m_Data.data(), sizeof(m_Data[0]), m_Data.size(), file);
  fwrite<uint32_t>(file, objects.size());
  for (const auto& object : objects) {
    fwrite<int32_t>(file, object.position.x);
    fwrite<int32_t>(file, object.position.y);
    fwrite<uint32_t>(file, object.components.size());
    for (const auto& component : object.components) {
      fwritestr(file, component.path.string());
      for (const auto& property : component.properties) fwritestr(file, property);
    }
  }
  fclose(file);
  project.closeAtlas(atlas);
}

IComponent::IComponent(std::filesystem::path path, std::vector<std::string> properties) : path(std::move(path)), properties(std::move(properties)) {
  this->properties.reserve(project.component(this->path).properties.size());
  while (this->properties.size() < project.component(this->path).properties.size()) {
    this->properties.emplace_back("");
  }
}

Atlas* Object::getAtlas() {
  for (auto& component : components) {
    if (component.path == "builtin/atlas") return &project.atlas(component.properties[0]);
  }
  return nullptr;
}
#pragma endregion MapFunctions
