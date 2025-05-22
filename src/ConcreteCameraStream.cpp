#include "ConcreteCameraStream.hpp"
// ICameraStream.cpp will contain the Config::render, so no imgui here unless for other reasons
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <vector>

// --- ConcreteCameraStream Implementation ---

std::string ConcreteCameraStream::buildErrorMessage(const char *operation, const std::string &details) {
  std::string msg = "ConcreteCameraStream: Operation '" + std::string(operation) + "' failed.";
  if (!details.empty()) {
    msg += " Details: " + details;
  }
  return msg;
}

ConcreteCameraStream::ConcreteCameraStream()
    : rawBuffer(nullptr), width(0), height(0), initialized(false), running(false), currentVerboseSetting(false) { // Default verbose to false
  spdlog::debug("ConcreteCameraStream: Initializing...");

  // Create the common config object, passing 'this' ConcreteCameraStream instance
  configController = std::make_shared<ICameraStream::Config>(this);

  try {
    camera.emplace(Metavision::Camera::from_first_available());
  } catch (const std::exception &e) {
    spdlog::error("ConcreteCameraStream: Failed to create Metavision::Camera: {}", e.what());
    throw std::runtime_error(buildErrorMessage("initialize (camera creation)", e.what()));
  }

  if (!camera) {
    spdlog::error("ConcreteCameraStream: Metavision::Camera is null after attempting creation.");
    throw std::runtime_error(buildErrorMessage("initialize (camera null check)"));
  }

  width = camera->geometry().get_width();
  height = camera->geometry().get_height();
  cvSize = cv::Size(width, height);

  if (width == 0 || height == 0) {
    camera.reset();
    spdlog::error("ConcreteCameraStream: Invalid camera geometry (width or height is 0).");
    throw std::runtime_error(buildErrorMessage("initialize (invalid geometry)"));
  }

  try {
    rawBuffer = new uint8_t[static_cast<size_t>(width) * height * 3];
    std::fill_n(rawBuffer, static_cast<size_t>(width) * height * 3, 0);
    cvImage.emplace(height, width, CV_8UC3, rawBuffer);
  } catch (const std::bad_alloc &e) {
    camera.reset();
    delete[] rawBuffer;
    rawBuffer = nullptr;
    spdlog::error("ConcreteCameraStream: Failed to allocate raw buffer or cv::Mat: {}", e.what());
    throw std::runtime_error(buildErrorMessage("initialize (buffer allocation)", e.what()));
  }

  initialized = true;
  // Set initial verbose state based on spdlog's current global level, or a default.
  // currentVerboseSetting is false by default. setVerbose will update it.
  // spdlog::get_level() could be used to sync if desired.
  spdlog::debug("ConcreteCameraStream: Initialized successfully (Width: {}, Height: {}).", width, height);
}

void ConcreteCameraStream::performCleanup() {
  if (camera && camera->is_running()) {
    try {
      camera->stop();
    } catch (const std::exception &e) {
      spdlog::error("ConcreteCameraStream: Exception during camera->stop(): {}", e.what());
    }
  }
  camera.reset();

  if (pool) {
    try {
      pool->shutdown();
    } catch (const std::exception &e) {
      spdlog::error("ConcreteCameraStream: Exception during pool->shutdown(): {}", e.what());
    }
    pool.reset();
  }

  cvImage.reset();

  delete[] rawBuffer;
  rawBuffer = nullptr;

  if (mainProcessingThread && mainProcessingThread->joinable()) {
    try {
      mainProcessingThread->join();
    } catch (const std::system_error &e) {
      spdlog::error("ConcreteCameraStream: Exception joining main processing thread: {}", e.what());
    }
  }
  mainProcessingThread.reset();

  initialized = false;
  running = false;
  spdlog::debug("ConcreteCameraStream: Cleanup performed.");
}

ConcreteCameraStream::~ConcreteCameraStream() {
  spdlog::debug("ConcreteCameraStream: Destructor called.");
  if (running) {
    running = false;
  }
  performCleanup();
}

std::shared_ptr<ICameraStream::Config> ConcreteCameraStream::getConfig() { return configController; }

void ConcreteCameraStream::setVerbose(bool verbose_flag) {
  try {
    spdlog::set_level(verbose_flag ? spdlog::level::debug : spdlog::level::info);
    currentVerboseSetting = verbose_flag; // Update internal state
    spdlog::debug("ConcreteCameraStream: Verbose logging set to {}. Internal state updated.", currentVerboseSetting);
  } catch (const std::exception &e) {
    spdlog::error("ConcreteCameraStream: Failed to set log level: {}", e.what());
    throw std::runtime_error(buildErrorMessage("setVerbose", e.what()));
  }
}

bool ConcreteCameraStream::getVerboseState() const { return currentVerboseSetting; }

// ... other ConcreteCameraStream methods (start, stop, get/setFadeTime, etc.) remain the same ...
// (Copying the rest from previous version for completeness)
void ConcreteCameraStream::start(uint32_t numThreads) {
  if (!initialized) {
    throw std::logic_error("ConcreteCameraStream: Cannot start, not initialized.");
  }
  if (running) {
    throw std::logic_error("ConcreteCameraStream: Already running.");
  }
  if (!camera) {
    throw std::logic_error("ConcreteCameraStream: Camera not available for starting.");
  }

  spdlog::debug("ConcreteCameraStream: Starting with {} threads...", numThreads);
  running = true;

  try {
    pool.emplace(numThreads, cvSize);

    auto eventCallback = [this](const Metavision::EventCD *begin, const Metavision::EventCD *end) {
      if (pool && running) {
        try {
          pool->addTask({begin, end});
        } catch (const std::exception &e) {
          spdlog::error("ConcreteCameraStream: Failed to add task to ThreadPool: {}", e.what());
        }
      }
    };

    camera->cd().add_callback(eventCallback);
    camera->start();

    auto mainWorker = [this]() {
      while (running) {
        if (pool && cvImage) {
          try {
            pool->sum(*cvImage);
          } catch (const std::exception &e) {
            spdlog::error("ConcreteCameraStream: Exception in mainWorker pool->sum(): {}", e.what());
          }
        }
      }
      spdlog::debug("ConcreteCameraStream: Main worker loop finished.");
    };
    mainProcessingThread.emplace(mainWorker);

  } catch (const std::exception &e) {
    spdlog::error("ConcreteCameraStream: Failed during start sequence: {}", e.what());
    running = false;
    performCleanup();
    throw std::runtime_error(buildErrorMessage("start", e.what()));
  }
  spdlog::debug("ConcreteCameraStream: Started successfully.");
}

void ConcreteCameraStream::stop() {
  if (!initialized && !running) {
    spdlog::debug("ConcreteCameraStream: Stop called but not initialized or not running.");
    return;
  }
  spdlog::debug("ConcreteCameraStream: Stopping...");
  running = false;

  try {
    performCleanup();
  } catch (const std::exception &e) {
    spdlog::error("ConcreteCameraStream: Exception during stop's performCleanup: {}", e.what());
    throw std::runtime_error(buildErrorMessage("stop (during cleanup)", e.what()));
  }
  spdlog::debug("ConcreteCameraStream: Stopped successfully.");
}

void ConcreteCameraStream::setFadeTime(uint32_t milliseconds) {
  if (!running || !pool) {
    throw std::logic_error("ConcreteCameraStream: Cannot setFadeTime, stream not running or pool not initialized.");
  }
  try {
    pool->setFadeTime(milliseconds);
    spdlog::debug("ConcreteCameraStream: Fade time set to {} ms", milliseconds);
  } catch (const std::exception &e) {
    spdlog::error("ConcreteCameraStream: Failed to setFadeTime on ThreadPool: {}", e.what());
    throw std::runtime_error(buildErrorMessage("setFadeTime (pool operation)", e.what()));
  }
}

uint32_t ConcreteCameraStream::getFadeTime() {
  if (!running || !pool) {
    throw std::logic_error("ConcreteCameraStream: Cannot getFadeTime, stream not running or pool not initialized.");
  }
  try {
    return pool->getFadeTime();
  } catch (const std::exception &e) {
    spdlog::error("ConcreteCameraStream: Failed to getFadeTime from ThreadPool: {}", e.what());
    throw std::runtime_error(buildErrorMessage("getFadeTime (pool operation)", e.what()));
  }
}

void ConcreteCameraStream::setFadeFrequency(uint32_t freq) {
  if (!running || !pool) {
    throw std::logic_error("ConcreteCameraStream: Cannot setFadeFrequency, stream not running or pool not initialized.");
  }
  try {
    pool->setFadeFrequency(freq);
    spdlog::debug("ConcreteCameraStream: Fade frequency set to {}", freq);
  } catch (const std::exception &e) {
    spdlog::error("ConcreteCameraStream: Failed to setFadeFrequency on ThreadPool: {}", e.what());
    throw std::runtime_error(buildErrorMessage("setFadeFrequency (pool operation)", e.what()));
  }
}

uint32_t ConcreteCameraStream::getFadeFrequency() {
  if (!running || !pool) {
    throw std::logic_error("ConcreteCameraStream: Cannot getFadeFrequency, stream not running or pool not initialized.");
  }
  try {
    return pool->getFadeFrequency();
  } catch (const std::exception &e) {
    spdlog::error("ConcreteCameraStream: Failed to getFadeFrequency from ThreadPool: {}", e.what());
    throw std::runtime_error(buildErrorMessage("getFadeFrequency (pool operation)", e.what()));
  }
}

uint8_t *ConcreteCameraStream::getBuffer() const { return rawBuffer; }

uint32_t ConcreteCameraStream::getWidth() const { return width; }

uint32_t ConcreteCameraStream::getHeight() const { return height; }

bool ConcreteCameraStream::isInitialized() const { return initialized; }

bool ConcreteCameraStream::isRunning() const { return running; }
