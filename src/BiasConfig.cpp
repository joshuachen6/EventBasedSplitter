#include "BiasConfig.hpp"

#include <imgui.h>
#include <metavision/hal/facilities/i_ll_biases.h>
#include <spdlog/spdlog.h>

BiasConfig::BiasConfig(std::optional<Metavision::Camera> *camera)
    : cameraPtr(camera), uiBiasDiff(BIAS_DIFF_DEFAULT), uiBiasDiffOn(BIAS_DIFF_ON_DEFAULT),
      uiBiasDiffOff(BIAS_DIFF_OFF_DEFAULT), uiBiasFo(BIAS_FO_DEFAULT), uiBiasHpf(BIAS_HPF_DEFAULT),
      uiBiasRefr(BIAS_REFR_DEFAULT) {
  if (cameraPtr && cameraPtr->has_value()) {
    reloadFromCamera();
  }
}

void BiasConfig::reloadFromCamera() {
  if (!cameraPtr || !cameraPtr->has_value()) {
    spdlog::warn("BiasConfig: Cannot reload biases, camera not available.");
    return;
  }

  int value;
  if (getBias("bias_diff", value)) {
    uiBiasDiff = value;
  }
  if (getBias("bias_diff_on", value)) {
    uiBiasDiffOn = value;
  }
  if (getBias("bias_diff_off", value)) {
    uiBiasDiffOff = value;
  }
  if (getBias("bias_fo", value)) {
    uiBiasFo = value;
  }
  if (getBias("bias_hpf", value)) {
    uiBiasHpf = value;
  }
  if (getBias("bias_refr", value)) {
    uiBiasRefr = value;
  }

  spdlog::debug("BiasConfig: Reloaded biases from camera.");
}

bool BiasConfig::applyBias(const std::string &biasName, int value) {
  if (!cameraPtr || !cameraPtr->has_value()) {
    spdlog::error("BiasConfig: Cannot apply bias '{}', camera not available.", biasName);
    return false;
  }

  try {
    auto *biases = (*cameraPtr)->get_device().get_facility<Metavision::I_LL_Biases>();
    if (!biases) {
      spdlog::error("BiasConfig: I_LL_Biases facility not available.");
      return false;
    }

    biases->set(biasName, value);
    spdlog::debug("BiasConfig: Set {} = {}", biasName, value);
    return true;
  } catch (const std::exception &e) {
    spdlog::error("BiasConfig: Failed to set bias '{}' to {}: {}", biasName, value, e.what());
    return false;
  }
}

bool BiasConfig::getBias(const std::string &biasName, int &outValue) {
  if (!cameraPtr || !cameraPtr->has_value()) {
    return false;
  }

  try {
    auto *biases = (*cameraPtr)->get_device().get_facility<Metavision::I_LL_Biases>();
    if (!biases) {
      return false;
    }

    outValue = biases->get(biasName);
    return true;
  } catch (const std::exception &e) {
    spdlog::warn("BiasConfig: Failed to get bias '{}': {}", biasName, e.what());
    return false;
  }
}

void BiasConfig::resetToDefaults() {
  uiBiasDiff = BIAS_DIFF_DEFAULT;
  uiBiasDiffOn = BIAS_DIFF_ON_DEFAULT;
  uiBiasDiffOff = BIAS_DIFF_OFF_DEFAULT;
  uiBiasFo = BIAS_FO_DEFAULT;
  uiBiasHpf = BIAS_HPF_DEFAULT;
  uiBiasRefr = BIAS_REFR_DEFAULT;

  // Apply all defaults to camera
  applyBias("bias_diff", uiBiasDiff);
  applyBias("bias_diff_on", uiBiasDiffOn);
  applyBias("bias_diff_off", uiBiasDiffOff);
  applyBias("bias_fo", uiBiasFo);
  applyBias("bias_hpf", uiBiasHpf);
  applyBias("bias_refr", uiBiasRefr);

  spdlog::info("BiasConfig: Reset all biases to defaults.");
}

void BiasConfig::render() {
  if (!cameraPtr || !cameraPtr->has_value()) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Camera not available for bias configuration.");
    return;
  }

  ImGui::PushID("BiasConfig");

  // Header with info
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Adjust camera sensor biases. Changes apply immediately.");
  ImGui::Spacing();

  // === bias_diff ===
  ImGui::Text("bias_diff:");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Base differential bias. Recommended to not change.\nDefault: %d, Range: %d to %d", BIAS_DIFF_DEFAULT,
                      BIAS_DIFF_MIN, BIAS_DIFF_MAX);
  }
  if (ImGui::SliderInt("##bias_diff", &uiBiasDiff, BIAS_DIFF_MIN, BIAS_DIFF_MAX)) {
    applyBias("bias_diff", uiBiasDiff);
  }

  // === bias_diff_on ===
  ImGui::Text("bias_diff_on:");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("ON event contrast threshold.\nDefault: %d, Range: %d to %d", BIAS_DIFF_ON_DEFAULT,
                      BIAS_DIFF_ON_MIN, BIAS_DIFF_ON_MAX);
  }
  if (ImGui::SliderInt("##bias_diff_on", &uiBiasDiffOn, BIAS_DIFF_ON_MIN, BIAS_DIFF_ON_MAX)) {
    applyBias("bias_diff_on", uiBiasDiffOn);
  }

  // === bias_diff_off ===
  ImGui::Text("bias_diff_off:");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("OFF event contrast threshold.\nDefault: %d, Range: %d to %d", BIAS_DIFF_OFF_DEFAULT,
                      BIAS_DIFF_OFF_MIN, BIAS_DIFF_OFF_MAX);
  }
  if (ImGui::SliderInt("##bias_diff_off", &uiBiasDiffOff, BIAS_DIFF_OFF_MIN, BIAS_DIFF_OFF_MAX)) {
    applyBias("bias_diff_off", uiBiasDiffOff);
  }

  ImGui::Separator();

  // === bias_fo ===
  ImGui::Text("bias_fo:");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Low-pass filter. Filters high-frequency light fluctuations.\nDefault: %d, Range: %d - %d",
                      BIAS_FO_DEFAULT, BIAS_FO_MIN, BIAS_FO_MAX);
  }
  if (ImGui::SliderInt("##bias_fo", &uiBiasFo, BIAS_FO_MIN, BIAS_FO_MAX)) {
    applyBias("bias_fo", uiBiasFo);
  }

  // === bias_hpf ===
  ImGui::Text("bias_hpf:");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("High-pass filter. Reduces low-frequency background noise.\nDefault: %d, Range: %d - %d",
                      BIAS_HPF_DEFAULT, BIAS_HPF_MIN, BIAS_HPF_MAX);
  }
  if (ImGui::SliderInt("##bias_hpf", &uiBiasHpf, BIAS_HPF_MIN, BIAS_HPF_MAX)) {
    applyBias("bias_hpf", uiBiasHpf);
  }

  // === bias_refr ===
  ImGui::Text("bias_refr:");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Refractory period. Time after each event before next can be generated.\nDefault: %d, Range: %d - %d",
                      BIAS_REFR_DEFAULT, BIAS_REFR_MIN, BIAS_REFR_MAX);
  }
  if (ImGui::SliderInt("##bias_refr", &uiBiasRefr, BIAS_REFR_MIN, BIAS_REFR_MAX)) {
    applyBias("bias_refr", uiBiasRefr);
  }

  ImGui::Spacing();
  ImGui::Separator();

  // Action buttons
  if (ImGui::Button("Reset to Defaults")) {
    resetToDefaults();
  }
  ImGui::SameLine();
  if (ImGui::Button("Reload from Camera")) {
    reloadFromCamera();
  }

  ImGui::PopID();
}
