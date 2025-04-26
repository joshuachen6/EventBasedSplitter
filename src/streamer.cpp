#include "streamer.h"

#include <chrono>
#include <exception>
#include <metavision/sdk/stream/camera.h>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

// Global variables
static cv::Mat *image;
static Metavision::Camera *camera;
static bool running = false;
static bool initialized = false;

int initialize(uint8_t **buffer, uint8_t *width, uint8_t *height) {
  try {
    // Create the camera
    *camera = Metavision::Camera::from_first_available();
  } catch (const std::exception &e) {
    // Catch any errors and log
    spdlog::error(e.what());
    return -1; 
  }

  // Get the dimensions and assign them
  *width = camera->geometry().get_width();
  *height = camera->geometry().get_height();

  // Create the image
  *image = cv::Mat::zeros({*width, *height}, CV_8UC3);

  // Assign the buffer
  *buffer = image->data;

  // Now initialized
  initialized = true;

  spdlog::debug("Camera initialized");
  return 0;
}

int start() {
  // Check if initialized
  if (not initialized) {
    spdlog::error("Camera has not been initialized yet");
    return -1;
  }

  if (running) {
    spdlog::error("Already running");
    return -1;
  }

  // The start time
  auto startTime = std::chrono::high_resolution_clock::now();

  // Create the event processor
  auto eventProcessor = [&](const Metavision::EventCD *begin,
                    const Metavision::EventCD *end) {
    // Loop through all the events
    for (auto it = begin; it != end; ++it) {
      // Write into raw
      auto *ptr = image->ptr<cv::Vec3b>(it->y, it->x);
      (*ptr)[0] = it->p * 255;
      (*ptr)[1] = (1 - it->p) * 255;
    }
  };

  // Create the event callback
  auto eventCallback = [&](const Metavision::EventCD *begin,
                      const Metavision::EventCD *end) {
    // Get the time since start
    long dt = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::high_resolution_clock::now() - startTime)
                  .count();

    // Check if the events have expired
    if ((dt - end->t) < 1e5) {
      // Read in another thread
      std::thread thread(eventProcessor, begin, end);
      thread.detach();
    }
  };

  // Create the main worker
  auto mainWorker = [&](){
    while (running) {
      // Fade the image
      *image/=2;
    }
  };


  // Register the callback
  try {
    camera->cd().add_callback(eventCallback);
    camera->start();
  } catch (const std::exception &e) {
    spdlog::error(e.what());
    return -1;
  }

  // Create the main worker thread
  std::thread mainWorkerThread(mainWorker);
  mainWorkerThread.detach();

  // Now running
  running = true;

  spdlog::debug("Camera started");

  return 0;
}

int stop() {
  // Clean up
  delete camera;
  delete image;

  // Reset global variables
  initialized = false;
  running = false;

  spdlog::debug("Camera stream stopped");
  return 0;
}
