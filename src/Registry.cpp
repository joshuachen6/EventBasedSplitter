#include "Registry.hpp"
#include <imgui.h>         // For ImGui::CollapsingHeader and other ImGui calls
#include <spdlog/spdlog.h> // For logging (optional, but good for diagnostics)

bool Registry::addConfig(const std::string &name, const std::shared_ptr<IConfig> &config) {
  if (name.empty()) {
    spdlog::warn("Registry: Attempted to add an IConfig with an empty name.");
    return false;
  }
  if (!config) {
    spdlog::warn("Registry: Attempted to add a null IConfig pointer for name '{}'.", name);
    return false;
  }

  // If a config with the same name exists, operator[] will overwrite it.
  configs[name] = config;
  spdlog::debug("Registry: Added/updated config component named '{}'.", name);
  return true;
}

std::shared_ptr<IConfig> Registry::getConfig(const std::string &name) const {
  auto it = configs.find(name);
  if (it != configs.end()) {
    return it->second;
  }
  spdlog::trace("Registry: Config component named '{}' not found.", name);
  return nullptr;
}

void Registry::renderImGui() {
  if (configs.empty()) {
    ImGui::Text("No configurations registered.");
    return;
  }

  // Iterate through all registered IConfig components.
  // std::map iterates in key-sorted order (alphabetical by name).
  for (const auto &pair : configs) {
    const std::string &configName = pair.first; // This is the name provided to addConfig
    const std::shared_ptr<IConfig> &configComponent = pair.second;

    if (configComponent) {
      // Each IConfig is rendered within its own collapsible header (fold)
      if (ImGui::CollapsingHeader(configName.c_str())) {
        // Push an ID to ensure widgets within different configs have unique IDs
        // if their internal labels might collide. Using the configName as part of the ID.
        ImGui::PushID(configName.c_str());

        configComponent->render(); // Delegate rendering to the IConfig component

        ImGui::PopID();
      }
    }
  }
}
