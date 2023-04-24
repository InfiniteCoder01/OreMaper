#include "00Names.hpp"

void Project::addBuiltinComponents() {
  componentCache["builtin/atlas"] = Component("builtin/atlas", {
                                                                   Property("Atlas", PropertyType::Atlas),
                                                               });
}
