#include "../00Names.hpp"
#include "../IconsFontAwesome6.h"

namespace Editor::Inspector {
std::string* map;
std::string* atlas;
bool addingComponent = false;
std::filesystem::path component;

void reset() {
  map = nullptr;
  atlas = nullptr;
  addingComponent = false;
  Mapper::currentObject = nullptr;
  component = "";
}

static void propertyValueInput(MvGuiTextInputState& state, std::string& value, PropertyType type) {
  if (type == PropertyType::Int) MvGui::TextInput(value, state, MvGuiTextInputType::Integer);
  else if (type == PropertyType::String) MvGui::TextInput(value, state, MvGuiTextInputType::Multiline);
  else if (type == PropertyType::Map) {
    if (&value == map) MvGui::TextUnformatted("choosing...");
    else if (MvGui::Button("Choose")) map = &value;
  } else if (type == PropertyType::Atlas) {
    if (&value == atlas) MvGui::TextUnformatted("choosing...");
    else if (MvGui::Button("Choose")) atlas = &value;
  }
}

void window() {
  MvGui::Begin("Inspector");

  // * Component
  static std::vector<MvGuiTextInputState> states;
  if (!component.empty() && !addingComponent) {
    MvGui::TextUnformatted(component.string());
    states.resize(project.component(component).properties.size());

    for (uint32_t i = 0; i < project.component(component).properties.size(); i++) {
      auto& property = project.component(component).properties[i];
      MvGui::TextInput(property.name, states[i]);
      MvGui::sameLine();
      if (MvGui::Button(types[(uint32_t)property.type])) property.type = (PropertyType)(((uint32_t)property.type + 1) % (uint32_t)PropertyType::TypeCount);
    }

    return MvGui::End();
  }

  // * Object
  if (Mapper::currentObject != nullptr) {
    const vec2f position = Mapper::currentObject->position / vec2f(Mapper::currentMap().getAtlas().tileSize);
    MvGui::Text("Object at (%0.1f, %0.1f)", position.x, position.y);
    MvGui::Separator();

    {
      uint32_t propertyCount = 0;
      for (const auto& component : Mapper::currentObject->components) propertyCount += component.properties.size();
      states.resize(propertyCount);
    }

    uint32_t propertyIndex = 0;
    for (uint32_t i = 0; i < Mapper::currentObject->components.size(); i++) {
      MvGui::TextUnformatted(Mapper::currentObject->components[i].path.string());
      MvGui::sameLine();

      // * Move Up
      if (i > 0) {
        if (MvGui::Button(ICON_FA_ARROW_UP)) std::iter_swap(Mapper::currentObject->components.begin() + i, Mapper::currentObject->components.begin() + i - 1);
        MvGui::sameLine();
      }

      // * Move Down
      if (i < Mapper::currentObject->components.size() - 1) {
        if (MvGui::Button(ICON_FA_ARROW_DOWN)) std::iter_swap(Mapper::currentObject->components.begin() + i, Mapper::currentObject->components.begin() + i + 1);
        MvGui::sameLine();
      }

      // * Delete
      if (MvGui::Button(ICON_FA_DELETE_LEFT)) Mapper::currentObject->components.erase(Mapper::currentObject->components.begin() + i--);

      // * Properties
      auto parent = project.component(Mapper::currentObject->components[i].path);
      for (uint32_t j = 0; j < Mapper::currentObject->components[i].properties.size(); j++) {
        MvGui::TextUnformatted(parent.properties[j].name);
        MvGui::sameLine();
        propertyValueInput(states[propertyIndex], Mapper::currentObject->components[i].properties[j], parent.properties[j].type);
        propertyIndex++;
      }
      MvGui::Separator();
    }
    {
      if (!addingComponent && MvGui::Button("Add Component")) addingComponent = true;
      if (addingComponent && !component.empty()) {
        Mapper::currentMap().objects.rbegin()->components.emplace_back(component);
        addingComponent = false;
        component = "";
      }
    }

    if (MvGui::Button("Delete")) {
      Mapper::currentMap().objects.erase(std::remove_if(Mapper::currentMap().objects.begin(), Mapper::currentMap().objects.end(),
                                             [](Object& object) {
                                               return &object == Mapper::currentObject;
                                             }),
          Mapper::currentMap().objects.end());
      Mapper::currentObject = nullptr;
    }

    return MvGui::End();
  }

  // * Nothing
  reset();
  MvGui::TextUnformatted("Nothing is selected");
  MvGui::End();
}
} // namespace Editor::Inspector
