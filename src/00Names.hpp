#include <filesystem>
#include <movaGUI.hpp>

using namespace Math;
using namespace VectorMath;

#pragma region File
static std::string freadstr(FILE* file) {
  std::string str;
  while (true) {
    const char c = fgetc(file);
    if (c == EOF || c == '\0') return str;
    str += c;
  }
  return str;
}

static void fwritestr(FILE* file, const std::string& str) { fwrite(str.c_str(), str.size() + 1, 1, file); }

template <typename T> static T fread(FILE* file) {
  T value;
  fread(&value, sizeof(value), 1, file);
  return value;
}

template <typename T> static void fwrite(FILE* file, const T& value) { fwrite(&value, sizeof(value), 1, file); }
#pragma endregion File
#pragma region Atlas
struct Atlas {
  Atlas() = default;
  Atlas(const std::filesystem::path& path) : texture(path), path(path) {
    this->path.replace_extension(".atl");
    FILE* file = fopen(this->path.string().c_str(), "rb");
    fread(&tileSize, sizeof(tileSize), 1, file);
    fclose(file);
  }

  ~Atlas() {
    FILE* file = fopen(path.string().c_str(), "wb");
    fwrite(&tileSize, sizeof(tileSize), 1, file);
    fclose(file);
  }

  MvImage texture;
  vec2u tileSize;

  MvImage& operator()() { return texture; }

private:
  std::filesystem::path path;
};

namespace Editor::AtlasView {
extern std::filesystem::path atlas;
extern Rect<uint32_t> selection;

void reset();
void window();
inline void open(std::filesystem::path path) {
  reset();
  atlas = path;
}

std::string exportAtlases();
} // namespace Editor::AtlasView
#pragma endregion Atlas
#pragma region Component
enum class PropertyType : uint8_t { Int, String, Map, Atlas, TypeCount };
const std::array<std::string, (uint32_t)PropertyType::TypeCount> types = {"int", "String", "Map", "Atlas"};

struct Property {
  std::string name;
  PropertyType type;

  Property() = default;
  Property(std::string name, PropertyType type) : name(std::move(name)), type(type) {}
};

struct Component {
  std::filesystem::path path;
  std::vector<Property> properties;

  Component() = default;
  Component(std::filesystem::path path, std::vector<Property> properties) : path(std::move(path)), properties(std::move(properties)) {}
  Component(std::filesystem::path path) : path(std::move(path)) {
    FILE* file = fopen(this->path.string().c_str(), "rb");
    properties.resize(fread<uint32_t>(file));
    for (auto& property : properties) {
      property.name = freadstr(file);
      property.type = fread<PropertyType>(file);
    }
    fclose(file);
  }

  ~Component() {
    if (path.string().substr(0, 8) == "builtin/") return;
    FILE* file = fopen(this->path.string().c_str(), "wb");
    fwrite<uint32_t>(file, properties.size());
    for (auto& property : properties) {
      fwritestr(file, property.name);
      fwrite<PropertyType>(file, property.type);
    }
    fclose(file);
  }
};

struct IComponent {
  std::filesystem::path path;
  std::vector<std::string> properties;

  IComponent() = default;
  IComponent(std::filesystem::path path, std::vector<std::string> properties = {});
};

struct Object {
  vec2i position;
  std::vector<IComponent> components;

  Object() = default;
  Object(vec2i position) : position(position) {}
  Atlas* getAtlas();
};
#pragma endregion Component
#pragma region Map
struct Map {
  vec2u size;

  Map() = default;
  Map(std::filesystem::path path);
  Map(uint32_t width, uint32_t height, std::filesystem::path atlas, std::filesystem::path path) : size(width, height), atlas(std::move(atlas)), m_Path(std::move(path)), m_Data(width * height, -1) {}
  ~Map();

  Atlas& getAtlas() const;

  vec2i operator()(vec2u position) { return operator()(position.x, position.y); }
  vec2i operator()(uint32_t x, uint32_t y) {
    if (x > size.x || y > size.y) return {-1};
    return m_Data[x + y * size.x];
  }

  void set(vec2u position, vec2i tile) { set(position.x, position.y, tile); }
  void set(uint32_t x, uint32_t y, vec2i tile) {
    if (x > size.x || y > size.y) return;
    m_Data[x + y * size.x] = tile;
  }

  std::vector<Object> objects;
  std::filesystem::path atlas;

private:
  std::vector<vec2u> m_Data;
  std::filesystem::path m_Path;
};

namespace Editor::Mapper {
extern Object* currentObject;
extern std::filesystem::path map;
Map& currentMap();

void reset();
void window();
inline void open(std::filesystem::path path) {
  reset();
  map = path;
}

std::string exportMaps();
} // namespace Editor::Mapper
#pragma endregion Map
#pragma region Views
namespace Editor::Assets {
void reset();
void window();
} // namespace Editor::Assets

namespace Editor::Inspector {
extern std::string* map;
extern std::string* atlas;
extern bool addingComponent;
extern std::filesystem::path component;

void reset();
void window();
} // namespace Editor::Inspector
#pragma endregion Views
#pragma region Project
struct Project {
  void addBuiltinComponents();
  void load(const std::filesystem::path& path) {
    Editor::Mapper::reset();
    Editor::Inspector::reset();
    Editor::Assets::reset();
    Editor::AtlasView::reset();
    mapCache.clear();
    componentCache.clear();
    atlasCache.clear();
    this->path = path;
    addBuiltinComponents();
  }

  Atlas& atlas(const std::filesystem::path& path) {
    auto it = atlasCache.find(path);
    if (it != atlasCache.end()) return it->second;
    return atlasCache.emplace(path, this->path / path).first->second;
  }

  void closeAtlas(const std::filesystem::path& path) { atlasCache.erase(path); }

  Map& map(const std::filesystem::path& path) {
    auto it = mapCache.find(path);
    if (it != mapCache.end()) return it->second;
    return mapCache.emplace(path, this->path / path).first->second;
  }

  void closeMap(const std::filesystem::path& path) { mapCache.erase(path); }

  Component& component(const std::filesystem::path& path) {
    auto it = componentCache.find(path);
    if (it != componentCache.end()) return it->second;
    return componentCache.emplace(path, this->path / path).first->second;
  }

  void closeComponent(const std::filesystem::path& path) { componentCache.erase(path); }

  std::string exportAll() {
    std::string result = "const uint8_t PROGMEM data[] = {\n  ";
    result += Editor::AtlasView::exportAtlases();

    {
      componentIndex.clear();
      componentIndex[std::filesystem::path("builtin/atlas")] = 0;
      uint32_t index = 1;
      for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.path().extension() != ".cmp") continue;
        componentIndex[entry.path().lexically_relative(path)] = index;
        index++;
      }
    }

    result += Editor::Mapper::exportMaps();
    for (uint8_t i = 0; i < 4; i++) result.pop_back();
    result += "\n};\n";
    return result;
  }

  std::map<std::filesystem::path, uint32_t> mapIndex;
  std::map<std::filesystem::path, uint32_t> atlasIndex;
  std::map<std::filesystem::path, uint32_t> componentIndex;

  std::filesystem::path path;
  std::map<std::filesystem::path, Atlas> atlasCache;
  std::map<std::filesystem::path, Map> mapCache;
  std::map<std::filesystem::path, Component> componentCache;
};

extern Project project;
#pragma endregion Project
#pragma region Export
static void uintToBytes(std::string& str, uint32_t value, uint8_t bytes) {
  for (uint32_t i = 0; i < bytes; i++) {
    str += std::to_string(value & 0xff) + ", ";
    value >>= 8;
  }
}

static std::string uintToBytes(uint32_t value, uint8_t bytes) {
  std::string str;
  uintToBytes(str, value, bytes);
  return str;
}
#pragma endregion Export

namespace Editor {
void windows();
}

// * Misc
std::filesystem::path getProgramPath();
