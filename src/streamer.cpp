#include "streamer.h"

#include "ThreadPool.h"
#include <cstdint>
#include <exception>
#include <metavision/sdk/stream/camera.h>
#include <opencv2/opencv.hpp>
#include <optional>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

// Global variables
static std::optional<cv::Mat> image;
static std::optional<Metavision::Camera> camera;
static std::optional<ThreadPool> pool;
static uint8_t *rawBuffer = nullptr;
static bool running = false;
static bool initialized = false;
static cv::Size size;

int initialize(uint8_t **buffer, uint32_t *width, uint32_t *height) {
  // Check to see if already initialized
  if (initialized) {
    spdlog::error("Already initialized");
    return -1;
  }

  try {
    // Create the camera
    camera = Metavision::Camera::from_first_available();
  } catch (const std::exception &e) {
    // Catch any errors and log
    spdlog::error(e.what());
    return -1;
  }

  // Get the dimensions and assign them
  *width = camera->geometry().get_width();
  *height = camera->geometry().get_height();

  size = cv::Size(*width, *height);

  // Create the raw buffer
  rawBuffer = new uint8_t[*width * *height * 3];
  std::fill_n(rawBuffer, *width * *height * 3, 0);

  // Create the image
  image = cv::Mat(*height, *width, CV_8UC3, rawBuffer);

  // Assign the buffer
  *buffer = rawBuffer;

  // Now initialized
  initialized = true;

  spdlog::debug("Camera initialized");
  return 0;
}

int start(uint32_t numThreads) {
  // Check if initialized
  if (not initialized) {
    spdlog::error("Camera has not been initialized yet");
    return -1;
  }

  if (running) {
    spdlog::error("Already running");
    return -1;
  }

  // Now running
  running = true;

  // Create the pool
  pool.emplace(numThreads, size);

  // Create the event callback
  auto eventCallback = [&](const Metavision::EventCD *begin,
                           const Metavision::EventCD *end) {
    // Submit task
    pool->addTask({begin, end});
  };

  // Register the callback
  try {
    camera->cd().add_callback(eventCallback);
    camera->start();
  } catch (const std::exception &e) {
    spdlog::error(e.what());
    return -1;
  }

  // Create the main worker
  auto mainWorker = [&]() {
    while (running) {
      // Fade the image
      pool->sum(*image);
    }
  };

  // Create the main worker thread
  std::thread mainWorkerThread(mainWorker);
  mainWorkerThread.detach();

  spdlog::debug("Camera started");

  return 0;
}

int stop() {
  // Check if initialized
  if (not initialized) {
    spdlog::error("Not initialized");
    return -1;
  }

  // Clean up if needed
  if (camera) {
    camera.reset();
  }
  if (image) {
    image.reset();
  }
  if (pool) {
    pool.reset();
  }
  if (rawBuffer) {
    delete[] rawBuffer;
    rawBuffer = nullptr;
  }

  // Reset global variables
  initialized = false;
  running = false;

  spdlog::debug("Camera stream stopped");
  return 0;
}

int setVerbose(bool verbose) {
  spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::info);
  return 0;
}

int setFadeTime(uint32_t milliseconds) {
  if (not running) {
    spdlog::error("Not running");
    return -1;
  }

  pool->setFadeTime(milliseconds);
  return 0;
}

int getFadeTime(uint32_t *fadeTime) {
  if (not running) {
    spdlog::error("Not running");
    return -1;
  }

  *fadeTime = pool->getFadeTime();
  return 0;
}
