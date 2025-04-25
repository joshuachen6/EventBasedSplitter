#include "streamer.h"
#include "util.h"
#include <metavision/sdk/base/events/event_cd.h>
#include <metavision/sdk/stream/camera.h>
#include <opencv2/core.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <opencv2/highgui.hpp>

// Declare the buffers
static uint8_t *rawBuffer, *topLeftBuffer, *topRightBuffer, *bottomLeftBuffer, *bottomRightBuffer, *centerBuffer;

// Declare the runtime variables
static uint8_t ROIOffset = 0;
static uint8_t ROIScale = 20;
static uint8_t ROISize = 20;

// The camera
static Metavision::Camera *camera;

// Program status
static bool running = false;

void init(uint8_t *raw, uint8_t *topLeft, uint8_t *topRight, uint8_t *bottomLeft, uint8_t *bottomRight, uint8_t *center) {
  rawBuffer = raw;
  topLeftBuffer = topLeft;
  topRightBuffer = topRight;
  bottomLeftBuffer = bottomLeft;
  bottomRightBuffer = bottomRight;
  centerBuffer = center;
  running = true;
}

uint8_t getROIOffset() {
  return ROIOffset;
}

uint8_t getROIScale() {
  return ROIScale;
}

uint8_t getROISize() {
  return ROISize;
}

void setROIOffset(uint8_t offset) {
  ROIOffset = offset;
  spdlog::info("Set ROI offset to {}", offset);
}

void setROIScale(uint8_t scale) {
  ROIScale = scale;
  spdlog::info("Set ROI scale to {}", scale);
}

void setROISize(uint8_t size) {
  ROISize = size;
  spdlog::info("Set ROI size to {}", size);
}

void start() {
  // Start the camera
  spdlog::info("Starting camera");
  Metavision::Camera camera = Metavision::Camera::from_first_available();
  spdlog::info("Successfully started camera");

  // Get the dimensions of the camera
  int width = camera.geometry().get_width();
  int height = camera.geometry().get_height();
  spdlog::info("Width {}, Height {}", width, height);

  // Create buffers for the image
  cv::Mat *raw = new cv::Mat(height, width, CV_8UC3, rawBuffer);
  
  // Counter to keep track of the event rate
  std::chrono::time_point start = std::chrono::high_resolution_clock::now();

  // Worker to write to the buffer
  auto worker = [&](const Metavision::EventCD *begin, const Metavision::EventCD *end) {
    // Loop through all the events
    for (auto it = begin; it != end; ++it) {
      // Write into raw
      cv::Vec3b *ptr = raw->ptr<cv::Vec3b>(it->y, it->x);
      (*ptr)[0] = it->p * 255;
      (*ptr)[1] = (1 - it->p) * 255;
    }
  };

  // Callback to be called with events
  auto callback = [&](const Metavision::EventCD *begin, const Metavision::EventCD *end) {
    long dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

    if ((dt - end->t) < 1e5) {
      std::thread thread(worker, begin, end);
      thread.detach();
    }
  };

  // Register the callback
  spdlog::info("Starting the camera stream");
  camera.cd().add_callback(callback);
  camera.start();
  spdlog::info("Successfully started camera stream");

  auto mainTask = [&]() {
    while (running) {
      uint8_t windowSize = ROISize * ROIScale;
      cutRegion(*raw, {0, 0, ROISize, ROISize}, ROIScale).copyTo(*(new cv::Mat(windowSize, windowSize, CV_8UC1, topLeftBuffer)));
      cutRegion(*raw, {width - ROISize, 0, ROISize, ROISize}, ROIScale).copyTo(*(new cv::Mat(windowSize, windowSize, CV_8UC1, topRightBuffer)));
      cutRegion(*raw, {0, height - ROISize, ROISize, ROISize}, ROIScale).copyTo(*(new cv::Mat(windowSize, windowSize, CV_8UC1, bottomLeftBuffer)));
      cutRegion(*raw, {width - ROISize, height - ROISize, ROISize, ROISize}, ROIScale).copyTo(*(new cv::Mat(windowSize, windowSize, CV_8UC1, bottomRightBuffer)));
      cutRegion(*raw, {(width - ROISize) / 2, (height - ROISize) / 2, ROISize, ROISize}, ROIScale).copyTo(*(new cv::Mat(windowSize, windowSize, CV_8UC1, centerBuffer)));

      // Fade
      *raw /= 2;
    }
    spdlog::info("Main loop shut down");
  };

  // std::thread mainTaskThread(mainTask);
  // mainTaskThread.detach();

  // Loop while running
  spdlog::info("Started main loop");
}

void stop() {
  spdlog::info("Exiting");

  // Shut down the camera
  spdlog::info("Shutting down the camera");
  camera->stop();
  spdlog::info("Successfully shut down the camera");

  spdlog::info("Shutting down main loop");
  running = false;
}
