#pragma once

#include "ICameraStream.hpp" // The interface it implements
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

class MockCameraStream : public ICameraStream {
public:
  MockCameraStream(uint32_t mockWidth = 640, uint32_t mockHeight = 480, uint32_t mockChannels = 3);
  ~MockCameraStream() override;

  MockCameraStream(const MockCameraStream &) = delete;
  MockCameraStream &operator=(const MockCameraStream &) = delete;
  MockCameraStream(MockCameraStream &&) = delete;
  MockCameraStream &operator=(MockCameraStream &&) = delete;

  // --- ICameraStream Interface Implementation ---
  std::shared_ptr<ICameraStream::Config> getConfig() override;

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
  void dataGenerationLoop();

  std::vector<uint8_t> bufferData;
  uint32_t streamWidth;
  uint32_t streamHeight;
  uint32_t streamChannels;

  std::atomic<bool> runningFlag;
  std::atomic<bool> initializedFlag;
  bool currentVerboseSetting; // Stores current verbose state

  std::thread dataThread;
  mutable std::mutex bufferMutex;

  uint32_t mockFadeTimeMs;
  uint32_t mockFadeFrequencyHz;

  std::mt19937 randomEngine;
  std::uniform_int_distribution<uint8_t> randomByteDistribution;

  std::shared_ptr<ICameraStream::Config> configController; // Instance of the common config
};
