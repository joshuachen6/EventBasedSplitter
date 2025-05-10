#include "streamer.h"

#include "ThreadPool.h"
#include <cstdint>
#include <exception>
#include <metavision/sdk/stream/camera.h>
#include <opencv2/opencv.hpp>
#include <optional>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

struct StreamerInstance {
  std::optional<cv::Mat> image;
  std::optional<Metavision::Camera> camera;
  std::optional<ThreadPool> pool;
  std::optional<std::thread> mainThread;
  uint8_t *rawBuffer = nullptr;
  bool running = false;
  bool initialized = false;
  cv::Size size;
};

int initialize(StreamerInstance **instance, uint8_t **buffer, uint32_t *width,
               uint32_t *height) {
  // Check if valid pointer
  if (not instance) {
    spdlog::error("Nullptr passed");
    return -1;
  }

  *instance = new StreamerInstance();

  try {
    // Create the camera
    (*instance)->camera = Metavision::Camera::from_first_available();
  } catch (const std::exception &e) {
    // Catch any errors and log
    spdlog::error(e.what());
    return -1;
  }

  // Get the dimensions and assign them
  *width = (*instance)->camera->geometry().get_width();
  *height = (*instance)->camera->geometry().get_height();

  (*instance)->size = cv::Size(*width, *height);

  // Create the raw buffer
  (*instance)->rawBuffer = new uint8_t[*width * *height * 3];
  std::fill_n((*instance)->rawBuffer, *width * *height * 3, 0);

  // Create the image
  (*instance)->image =
      cv::Mat(*height, *width, CV_8UC3, (*instance)->rawBuffer);

  // Assign the buffer
  *buffer = (*instance)->rawBuffer;

  // Now initialized
  (*instance)->initialized = true;

  spdlog::debug("Camera initialized");
  return 0;
}

int start(StreamerInstance *instance, uint32_t numThreads) {
  // Check if initialized
  if (not instance) {
    spdlog::error("Null streamer instance");
    return -1;
  }

  if (not instance->initialized) {
    spdlog::error("Camera has not been initialized yet");
    return -1;
  }

  if (instance->running) {
    spdlog::error("Already running");
    return -1;
  }

  // Now running
  instance->running = true;

  // Create the pool
  instance->pool.emplace(numThreads, instance->size);

  // Create the event callback
  auto eventCallback = [instance](const Metavision::EventCD *begin,
                                  const Metavision::EventCD *end) {
    // Submit task
    instance->pool->addTask({begin, end});
  };

  // Register the callback
  try {
    instance->camera->cd().add_callback(eventCallback);
    instance->camera->start();
  } catch (const std::exception &e) {
    spdlog::error(e.what());
    return -1;
  }

  // Create the main worker
  auto mainWorker = [instance]() {
    while (instance->running) {
      // Fade the image
      instance->pool->sum(*instance->image);
    }
  };

  // Create the main worker thread
  instance->mainThread.emplace(mainWorker);

  spdlog::debug("Camera started");

  return 0;
}

int stop(StreamerInstance *instance) {
  // Check if initialized
  if (not instance or not instance->initialized) {
    spdlog::error("Not initialized");
    return -1;
  }

  // Clean up if needed
  if (instance->camera) {
    instance->camera.reset();
  }
  if (instance->image) {
    instance->image.reset();
  }
  if (instance->pool) {
    instance->pool->shutdown();
    instance->pool.reset();
  }
  if (instance->rawBuffer) {
    delete[] instance->rawBuffer;
    instance->rawBuffer = nullptr;
  }
  if (instance->mainThread) {
    instance->mainThread->join();
    instance->mainThread.reset();
  }

  // Reset global variables
  instance->initialized = false;
  instance->running = false;

  delete instance;

  spdlog::debug("Camera stream stopped");
  return 0;
}

int setVerbose(StreamerInstance *instance, uint8_t verbose) {
  spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::info);
  return 0;
}

int setFadeTime(StreamerInstance *instance, uint32_t milliseconds) {
  if (not instance or not instance->running) {
    spdlog::error("Not running");
    return -1;
  }

  instance->pool->setFadeTime(milliseconds);
  return 0;
}

int getFadeTime(StreamerInstance *instance, uint32_t *fadeTime) {
  if (not instance or not instance->running) {
    spdlog::error("Not running");
    return -1;
  }

  *fadeTime = instance->pool->getFadeTime();
  return 0;
}
