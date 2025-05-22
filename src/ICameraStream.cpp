#include "ICameraStream.hpp"
#include <imgui.h>
#include <limits> // For std::numeric_limits
#include <spdlog/spdlog.h>

// --- ICameraStream::Config Implementation ---

ICameraStream::Config::Config(ICameraStream *parent) : parentStreamInstance(parent), uiVerbose(false), uiFadeTimeMs(1000), uiFadeFrequencyHz(30) {
  if (!parentStreamInstance) {
    spdlog::error("ICameraStream::Config: Initialized with a null parent ICameraStream pointer.");
    // This is a critical error for the config to function.
    // Throwing here ensures the config object isn't used in a bad state.
    throw std::invalid_argument("Parent stream instance cannot be null for ICameraStream::Config.");
  }
  loadInitialValues();
}

void ICameraStream::Config::loadInitialValues() {
  if (parentStreamInstance && parentStreamInstance->isInitialized()) {
    try {
      uiVerbose = parentStreamInstance->getVerboseState(); // Use the new getter

      // Fade time and frequency might only be valid if the stream is running
      if (parentStreamInstance->isRunning()) {
        uiFadeTimeMs = static_cast<int>(parentStreamInstance->getFadeTime());
        uiFadeFrequencyHz = static_cast<int>(parentStreamInstance->getFadeFrequency());
      } else {
        spdlog::debug("ICameraStream::Config: Stream not running, using default/last UI values for fade time/frequency.");
        // Keep current uiFadeTimeMs and uiFadeFrequencyHz or reset to defaults if preferred
      }
    } catch (const std::exception &e) {
      spdlog::error("ICameraStream::Config: Error loading initial values: {}", e.what());
    }
  } else {
    spdlog::debug("ICameraStream::Config: Parent stream is null or not initialized. Using default UI values.");
  }
}

void ICameraStream::Config::render() {
  if (!parentStreamInstance) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Parent ICameraStream is not available for Config.");
    return;
  }

  ImGui::PushID("ICameraStreamCommonConfig");

  if (ImGui::Checkbox("Enable Verbose Logging", &uiVerbose)) {
    try {
      parentStreamInstance->setVerbose(uiVerbose);
    } catch (const std::exception &e) {
      spdlog::error("ICameraStream::Config: Failed to set verbose mode: {}", e.what());
      // Optionally revert uiVerbose by calling loadInitialValues() or getting current state
      // uiVerbose = parentStreamInstance->getVerboseState();
    }
  }

  ImGui::Text("Fade Time (ms):");
  ImGui::SameLine();
  if (ImGui::DragInt("##ConfigFadeTime", &uiFadeTimeMs, 10.0f, 0, 60000)) {
    if (uiFadeTimeMs < 0)
      uiFadeTimeMs = 0;
    try {
      parentStreamInstance->setFadeTime(static_cast<uint32_t>(uiFadeTimeMs));
    } catch (const std::logic_error &e) {
      spdlog::warn("ICameraStream::Config: Could not set fade time (logic error): {}", e.what());
      if (parentStreamInstance->isRunning())
        uiFadeTimeMs = static_cast<int>(parentStreamInstance->getFadeTime()); // Revert
    } catch (const std::exception &e) {
      spdlog::error("ICameraStream::Config: Failed to set fade time: {}", e.what());
      if (parentStreamInstance->isRunning())
        uiFadeTimeMs = static_cast<int>(parentStreamInstance->getFadeTime()); // Revert
    }
  }

  ImGui::Text("Fade Frequency (Hz):");
  ImGui::SameLine();
  if (ImGui::DragInt("##ConfigFadeFrequency", &uiFadeFrequencyHz, 1.0f, 1, 240)) {
    if (uiFadeFrequencyHz < 1)
      uiFadeFrequencyHz = 1;
    try {
      parentStreamInstance->setFadeFrequency(static_cast<uint32_t>(uiFadeFrequencyHz));
    } catch (const std::logic_error &e) {
      spdlog::warn("ICameraStream::Config: Could not set fade frequency (logic error): {}", e.what());
      if (parentStreamInstance->isRunning())
        uiFadeFrequencyHz = static_cast<int>(parentStreamInstance->getFadeFrequency()); // Revert
    } catch (const std::exception &e) {
      spdlog::error("ICameraStream::Config: Failed to set fade frequency: {}", e.what());
      if (parentStreamInstance->isRunning())
        uiFadeFrequencyHz = static_cast<int>(parentStreamInstance->getFadeFrequency()); // Revert
    }
  }

  if (ImGui::Button("Refresh Config Values from Stream")) {
    loadInitialValues();
  }

  ImGui::PopID();
}
