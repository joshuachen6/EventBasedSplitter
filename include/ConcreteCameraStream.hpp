#pragma once

#include "ICameraStream.hpp" // Include the updated interface
#include "BiasConfig.hpp"    // Bias configuration UI

#include <cstdint>
#include <memory> // For std::shared_ptr, std::make_shared
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

#include "ThreadPool.h"
#include <metavision/sdk/stream/camera.h>
#include <opencv2/opencv.hpp>

class ConcreteCameraStream : public ICameraStream {
public:
  ConcreteCameraStream();
  ~ConcreteCameraStream() override;

  ConcreteCameraStream(const ConcreteCameraStream &) = delete;
  ConcreteCameraStream &operator=(const ConcreteCameraStream &) = delete;
  ConcreteCameraStream(ConcreteCameraStream &&) = delete;
  ConcreteCameraStream &operator=(ConcreteCameraStream &&) = delete;

  // --- ICameraStream Interface Implementation ---
  std::shared_ptr<ICameraStream::Config> getConfig() override;

  /**
   * @brief Gets the bias configuration UI for this camera stream.
   * @return A shared_ptr to the BiasConfig object.
   */
  std::shared_ptr<BiasConfig> getBiasConfig();

  void start(uint32_t numThreads) override;
  void stop() override;
  void setVerbose(bool verbose) override;
  bool getVerboseState() const override; // Implement this
  void setFadeTime(uint32_t milliseconds) override;
  uint32_t getFadeTime() override;
  void setFadeFrequency(uint32_t frequency) override;
  uint32_t getFadeFrequency() override;
  uint8_t *getBuffer() const override;
  uint32_t getWidth() const override;
  uint32_t getHeight() const override;
  bool isInitialized() const override;
  bool isRunning() const override;

private:
  std::optional<Metavision::Camera> camera;
  std::optional<ThreadPool> pool;
  std::optional<std::thread> mainProcessingThread;
  std::optional<cv::Mat> cvImage;

  uint8_t *rawBuffer;
  uint32_t width;
  uint32_t height;
  cv::Size cvSize;

  bool initialized;
  bool running;
  bool currentVerboseSetting; // Stores the current verbose state

  std::shared_ptr<ICameraStream::Config> configController; // Instance of the common config
  std::shared_ptr<BiasConfig> biasConfigController;        // Instance of the bias config

  static std::string buildErrorMessage(const char *operation, const std::string &details = "");
  void performCleanup();
};
