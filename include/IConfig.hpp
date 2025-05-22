#pragma once

#include <string> // Retained for potential use within render() method, e.g., for ImGui widget IDs.

/**
 * @brief Abstract interface for a configuration component.
 * Each IConfig implementer is responsible for holding its own values
 * and rendering its own ImGui interface for modification.
 * The name/title for this config in the UI is managed externally by the Registry.
 */
class IConfig {
public:
  /**
   * @brief Virtual destructor.
   */
  virtual ~IConfig() = default;

  /**
   * @brief Renders the ImGui interface for this configuration component.
   * This method will be called by the Registry to display and allow modification
   * of the component's settings.
   */
  virtual void render() = 0;
};
