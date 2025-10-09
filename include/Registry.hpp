#pragma once

#include "IConfig.hpp" // The updated interface
#include <map>
#include <memory> // For std::shared_ptr
#include <string>

/**
 * @brief A registry to hold and manage various IConfig components, associating them with names.
 * The Registry uses a map, so components will be ordered alphabetically by their given name.
 * It's responsible for rendering a top-level ImGui interface where each registered IConfig
 * component can be displayed and modified within its own collapsible section (fold).
 */
class Registry {
public:
  Registry() = default;

  // Prevent copying and moving for simplicity.
  Registry(const Registry &) = delete;
  Registry &operator=(const Registry &) = delete;
  Registry(Registry &&) = delete;
  Registry &operator=(Registry &&) = delete;

  /**
   * @brief Adds a configuration component to the registry with a specified name.
   * If a component with the same name already exists, it will be overwritten.
   * @param name The unique name to associate with this configuration component (used for UI and lookup).
   * @param config A shared pointer to the IConfig component to add.
   * @return True if the component was added successfully, false if the config is null or name is empty.
   */
  bool addConfig(const std::string &name, const std::shared_ptr<IConfig> &config);

  /**
   * @brief Retrieves a configuration component by its name.
   * @param name The name of the configuration component.
   * @return A shared_ptr to the IConfig component, or nullptr if not found.
   */
  std::shared_ptr<IConfig> getConfig(const std::string &name) const;

  /**
   * @brief Renders the ImGui interface for all registered configuration components.
   * Each component is rendered within a collapsible header (fold) titled with its associated name.
   * Components will be rendered in alphabetical order of their names due to std::map usage.
   */
  void renderImGui();

private:
  // Stores configuration components, mapped by their unique user-provided names.
  std::map<std::string, std::shared_ptr<IConfig>> configs;
};
