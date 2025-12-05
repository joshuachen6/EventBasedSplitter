#pragma once

#include "IConfig.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <metavision/sdk/stream/camera.h>

/**
 * @brief Configuration UI for Metavision camera biases.
 *
 * This class provides ImGui controls for adjusting camera bias parameters:
 * - bias_diff: Base differential bias (affects both ON and OFF thresholds)
 * - bias_diff_on: ON event contrast threshold (must be >= bias_diff + 15)
 * - bias_diff_off: OFF event contrast threshold (must be <= bias_diff - 15)
 * - bias_fo: Low-pass filter control (filters high-frequency fluctuations)
 * - bias_hpf: High-pass filter control (filters low-frequency background noise)
 * - bias_refr: Refractory period adjustment (time after each event)
 */
class BiasConfig : public IConfig {
public:
  /**
   * @brief Constructor for BiasConfig.
   * @param camera Pointer to an optional Metavision::Camera instance.
   *               The camera must be valid and initialized for bias control to work.
   */
  explicit BiasConfig(std::optional<Metavision::Camera> *camera);
  ~BiasConfig() override = default;

  BiasConfig(const BiasConfig &) = delete;
  BiasConfig &operator=(const BiasConfig &) = delete;
  BiasConfig(BiasConfig &&) = delete;
  BiasConfig &operator=(BiasConfig &&) = delete;

  /**
   * @brief Renders the ImGui interface for bias configuration.
   */
  void render() override;

  /**
   * @brief Reloads bias values from the camera hardware.
   * Call this after camera initialization to sync UI with actual values.
   */
  void reloadFromCamera();

private:
  std::optional<Metavision::Camera> *cameraPtr;

  // UI state for bias values
  int uiBiasDiff;
  int uiBiasDiffOn;
  int uiBiasDiffOff;
  int uiBiasFo;
  int uiBiasHpf;
  int uiBiasRefr;

  // Bias limits based on the provided specification
  static constexpr int BIAS_DIFF_DEFAULT = 0;
  static constexpr int BIAS_DIFF_MIN = -25;
  static constexpr int BIAS_DIFF_MAX = 23;

  static constexpr int BIAS_DIFF_ON_DEFAULT = 0;
  static constexpr int BIAS_DIFF_ON_MIN = -85;
  static constexpr int BIAS_DIFF_ON_MAX = 140;

  static constexpr int BIAS_DIFF_OFF_DEFAULT = 0;
  static constexpr int BIAS_DIFF_OFF_MIN = -35;
  static constexpr int BIAS_DIFF_OFF_MAX = 190;

  static constexpr int BIAS_FO_DEFAULT = 0;
  static constexpr int BIAS_FO_MIN = -35;
  static constexpr int BIAS_FO_MAX = 55;

  static constexpr int BIAS_HPF_DEFAULT = 0;
  static constexpr int BIAS_HPF_MIN = 0;
  static constexpr int BIAS_HPF_MAX = 120;

  static constexpr int BIAS_REFR_DEFAULT = 0;
  static constexpr int BIAS_REFR_MIN = -20;
  static constexpr int BIAS_REFR_MAX = 235;

  /**
   * @brief Applies a single bias value to the camera.
   * @param biasName The name of the bias (e.g., "bias_diff").
   * @param value The value to set.
   * @return true if successful, false otherwise.
   */
  bool applyBias(const std::string &biasName, int value);

  /**
   * @brief Gets the current value of a bias from the camera.
   * @param biasName The name of the bias.
   * @param outValue Output parameter for the retrieved value.
   * @return true if successful, false otherwise.
   */
  bool getBias(const std::string &biasName, int &outValue);

  /**
   * @brief Resets all biases to their default values.
   */
  void resetToDefaults();
};
