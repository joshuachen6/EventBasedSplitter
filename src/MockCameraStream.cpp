#include "ICameraStream.hpp" // For ICameraStream::Config
#include "MockCameraStream.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <vector>

MockCameraStream::MockCameraStream(uint32_t mockWidth, uint32_t mockHeight, uint32_t mockChannels)
    : streamWidth(mockWidth), streamHeight(mockHeight), streamChannels(mockChannels), runningFlag(false), initializedFlag(false),
      currentVerboseSetting(false), // Default verbose to false
      mockFadeTimeMs(1000), mockFadeFrequencyHz(30), randomEngine(std::random_device{}()) {
  configController = std::make_shared<ICameraStream::Config>(this);

  if (streamWidth == 0 || streamHeight == 0 || streamChannels == 0) {
    spdlog::error("MockCameraStream: Invalid dimensions (width, height, or channels cannot be 0).");
    throw std::invalid_argument("MockCameraStream: Dimensions cannot be zero.");
  }

  try {
    size_t bufferSize = static_cast<size_t>(streamWidth) * streamHeight * streamChannels;
    bufferData.resize(bufferSize);
    std::fill(bufferData.begin(), bufferData.end(), 0);
  } catch (const std::bad_alloc &e) {
    spdlog::error("MockCameraStream: Failed to allocate buffer: {}", e.what());
    throw std::runtime_error("MockCameraStream: Buffer allocation failed.");
  }

  randomByteDistribution = std::uniform_int_distribution<uint8_t>(0, 255);
  initializedFlag = true;
  spdlog::info("MockCameraStream: Initialized with size {}x{}x{}.", streamWidth, streamHeight, streamChannels);
}

MockCameraStream::~MockCameraStream() {
  spdlog::debug("MockCameraStream: Destructor called.");
  if (runningFlag.load()) {
    stop();
  }
}

std::shared_ptr<ICameraStream::Config> MockCameraStream::getConfig() { return configController; }

void MockCameraStream::dataGenerationLoop() {
  spdlog::debug("MockCameraStream: Data generation thread started.");
  while (runningFlag.load()) {
    {
      std::lock_guard<std::mutex> lock(bufferMutex);
      std::generate(bufferData.begin(), bufferData.end(), [this]() { return randomByteDistribution(randomEngine); });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(33));
  }
  spdlog::debug("MockCameraStream: Data generation thread stopped.");
}

void MockCameraStream::start(uint32_t numThreads) {
  if (!initializedFlag.load()) {
    spdlog::error("MockCameraStream: Cannot start, not initialized.");
    throw std::logic_error("MockCameraStream: Not initialized.");
  }
  if (runningFlag.load()) {
    spdlog::warn("MockCameraStream: Start called but already running.");
    return;
  }

  runningFlag = true;
  try {
    dataThread = std::thread(&MockCameraStream::dataGenerationLoop, this);
  } catch (const std::system_error &e) {
    runningFlag = false;
    spdlog::error("MockCameraStream: Failed to start data generation thread: {}", e.what());
    throw std::runtime_error("MockCameraStream: Failed to start data generation thread.");
  }
  spdlog::info("MockCameraStream: Started (mock data generation). numThreads hint (not used by mock): {}", numThreads);
}

void MockCameraStream::stop() {
  if (!runningFlag.load()) {
    spdlog::debug("MockCameraStream: Stop called but not running.");
    return;
  }
  spdlog::info("MockCameraStream: Stopping...");
  runningFlag = false;

  if (dataThread.joinable()) {
    try {
      dataThread.join();
    } catch (const std::system_error &e) {
      spdlog::error("MockCameraStream: Error joining data generation thread: {}", e.what());
      throw std::runtime_error("MockCameraStream: Error joining data thread.");
    }
  }
  spdlog::info("MockCameraStream: Stopped.");
}

void MockCameraStream::setVerbose(bool verbose) {
  currentVerboseSetting = verbose;
  // This mock doesn't use spdlog directly for its own operations as much,
  // but we can log this call.
  spdlog::info("MockCameraStream: Verbose output set to {}.", currentVerboseSetting);
  // If you wanted this mock to also control global spdlog level:
  // spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::info);
}

bool MockCameraStream::getVerboseState() const { return currentVerboseSetting; }

void MockCameraStream::setFadeTime(uint32_t milliseconds) {
  if (!initializedFlag.load()) { // Allow setting even if not running, but must be initialized
    spdlog::warn("MockCameraStream: setFadeTime called on non-initialized stream.");
    throw std::logic_error("MockCameraStream: Not initialized to set fade time.");
  }
  mockFadeTimeMs = milliseconds;
  spdlog::info("MockCameraStream: Mock fade time set to {} ms.", mockFadeTimeMs);
}

uint32_t MockCameraStream::getFadeTime() {
  if (!initializedFlag.load()) {
    spdlog::warn("MockCameraStream: getFadeTime called on non-initialized stream.");
    throw std::logic_error("MockCameraStream: Not initialized to get fade time.");
  }
  spdlog::debug("MockCameraStream: Getting mock fade time: {} ms.", mockFadeTimeMs);
  return mockFadeTimeMs;
}

void MockCameraStream::setFadeFrequency(uint32_t frequency) {
  if (!initializedFlag.load()) {
    spdlog::warn("MockCameraStream: setFadeFrequency called on non-initialized stream.");
    throw std::logic_error("MockCameraStream: Not initialized to set fade frequency.");
  }
  mockFadeFrequencyHz = frequency;
  spdlog::info("MockCameraStream: Mock fade frequency set to {} Hz.", mockFadeFrequencyHz);
}

uint32_t MockCameraStream::getFadeFrequency() {
  if (!initializedFlag.load()) {
    spdlog::warn("MockCameraStream: getFadeFrequency called on non-initialized stream.");
    throw std::logic_error("MockCameraStream: Not initialized to get fade frequency.");
  }
  spdlog::debug("MockCameraStream: Getting mock fade frequency: {} Hz.", mockFadeFrequencyHz);
  return mockFadeFrequencyHz;
}

uint8_t *MockCameraStream::getBuffer() const {
  if (!initializedFlag.load()) {
    return nullptr;
  }
  return const_cast<uint8_t *>(bufferData.data());
}

uint32_t MockCameraStream::getWidth() const { return streamWidth; }

uint32_t MockCameraStream::getHeight() const { return streamHeight; }

bool MockCameraStream::isInitialized() const { return initializedFlag.load(); }

bool MockCameraStream::isRunning() const { return runningFlag.load(); }
