#include "streamer.h"
#include <metavision/sdk/base/events/event_cd.h>
#include <metavision/sdk/stream/camera.h>
#include <omp-tools.h>
#include <omp.h>
#include <spdlog/spdlog.h>

// Declare the buffers
static cv::Mat rawBuffer, topLeftBuffer, topRightBuffer, bottomLeftBuffer,
    bottomRightBuffer;

// Declare the runtime variables
static uint8_t ROIOffset = 0;
static uint8_t ROIScale = 20;
static uint8_t ROISize = 20;

// The camera
static Metavision::Camera *camera;

void init(uint8_t *raw, uint8_t *topLeft, uint8_t *topRight,
          uint8_t *bottomLeft, uint8_t *bottomRight) {}

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
  // Create the camera
  spdlog::info("Starting the camera");
  *camera = Metavision::Camera::from_first_available();
  spdlog::info("Successfully started the camera");

  // Callback to be called with events
  auto callback = [&](const Metavision::EventCD *begin,
                      const Metavision::EventCD *end) {
// Loop through all the events
#pragma omp parallel for
    for (auto it = begin; it != end; ++it) {
      // Write into raw
      cv::Vec3b *ptr = rawBuffer.ptr<cv::Vec3b>(it->y, it->x);
      (*ptr)[0] = it->p * 255;
      (*ptr)[1] = (1 - it->p) * 255;
    }
  };

  // Register the callback
  spdlog::info("Starting the camera stream");
  camera->cd().add_callback(callback);
  camera->start();
  spdlog::info("Successfully started camera stream");
}

void stop() {}
