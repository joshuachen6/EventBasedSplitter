#include "util.h"
#include <atomic>
#include <chrono>
#include <metavision/hal/facilities/i_roi.h>
#include <metavision/sdk/base/events/event_cd.h>
#include <metavision/sdk/stream/camera.h>
#include <omp-tools.h>
#include <omp.h>
#include <opencv2/core.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

int main() {
  // Print diagnostic information
  spdlog::info("Max threads {}", omp_get_max_threads());

  // Start the camera
  spdlog::info("Starting camera");
  Metavision::Camera camera = Metavision::Camera::from_first_available();
  spdlog::info("Successfully started camera");

  // Get the dimensions of the camera
  int width = camera.geometry().get_width();
  int height = camera.geometry().get_height();
  spdlog::info("Width {}, Height {}", width, height);

  // Create buffers for the image
  cv::Mat raw = cv::Mat::zeros({width, height}, CV_8UC3);

  // Counter to keep track of the event rate
  std::chrono::time_point start = std::chrono::high_resolution_clock::now();

  auto worker = [&](const Metavision::EventCD *begin,
                    const Metavision::EventCD *end) {
    // Diagnostic
    long dt = std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::high_resolution_clock::now() - start)
                  .count();

    if ((dt - end->t) < 1e6) {

      // Loop through all the events
      for (auto it = begin; it != end; ++it) {
        // Write into raw
        cv::Vec3b *ptr = raw.ptr<cv::Vec3b>(it->y, it->x);
        (*ptr)[0] = it->p * 255;
        (*ptr)[1] = (1 - it->p) * 255;
      }
    }
  };

  // Callback to be called with events
  auto callback = [&](const Metavision::EventCD *begin,
                      const Metavision::EventCD *end) {
    std::thread thread(worker, begin, end);
    thread.detach();
  };

  // Register the callback
  spdlog::info("Starting the camera stream");
  camera.cd().add_callback(callback);
  camera.start();
  spdlog::info("Successfully started camera stream");

  // Loop while running
  spdlog::info("Started main loop");

  const int roiSize = 10;
  const int scaleFactor = 20;

  // Set up the regions of interest
  // Metavision::I_ROI &roi = camera.get_facility<Metavision::I_ROI>();
  // roi.set_windows({{0, 0, roiSize, roiSize},
  //                  {width - roiSize, 0, roiSize, roiSize},
  //                  {0, height - roiSize, roiSize, roiSize},
  //                  {width - roiSize, height - roiSize, roiSize, roiSize}});
  // roi.set_window({0, 0, roiSize, roiSize});
  // roi.enable(true);

  while (true) {
    // Get the submats

    // Show the image
    cv::imshow("Raw", raw);
    cv::imshow("top left",
               cutRegion(raw, {0, 0, roiSize, roiSize}, scaleFactor));
    cv::imshow(
        "top right",
        cutRegion(raw, {width - roiSize, 0, roiSize, roiSize}, scaleFactor));
    cv::imshow(
        "bottom left",
        cutRegion(raw, {0, height - roiSize, roiSize, roiSize}, scaleFactor));
    cv::imshow("bottom right",
               cutRegion(raw,
                         {width - roiSize, height - roiSize, roiSize, roiSize},
                         scaleFactor));

    // Wait for user input
    char input = cv::waitKey(1);

    // Exit
    if (input == 'q') {
      spdlog::info("User termination");
      break;
    }

    // Fade
    raw /= 2;
  }
  spdlog::info("Exiting");

  // Shut down the camera
  spdlog::info("Shutting down the camera");
  camera.stop();
  spdlog::info("Successfully shut down the camera");
}
