#include <metavision/sdk/base/events/event_cd.h>
#include <metavision/sdk/stream/camera.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <spdlog/spdlog.h>

int main() {
  // Start the camera
  spdlog::info("Starting camera");
  Metavision::Camera camera = Metavision::Camera::from_first_available();
  spdlog::info("Successfully started camera");

  // Get the dimensions of the camera
  int width = camera.geometry().get_width();
  int height = camera.geometry().get_height();
  spdlog::info("Width {}, Height {}", width, height);

  // Create buffers for the image
  cv::Mat raw = cv::Mat::zeros({width, height}, CV_8U);
  cv::Mat topLeft = cv::Mat::zeros({width / 2, height / 2}, CV_8U);
  cv::Mat topRight = cv::Mat::zeros({width / 2, height / 2}, CV_8U);
  cv::Mat bottomLeft = cv::Mat::zeros({width / 2, height / 2}, CV_8U);
  cv::Mat bottomRight = cv::Mat::zeros({width / 2, height / 2}, CV_8U);

  // Callback to be called with events
  auto callback = [&](const Metavision::EventCD *begin,
                      const Metavision::EventCD *end) {
  // Loop through all the events
#pragma opm parallel_for
    for (auto it = begin; it != end; ++it) {
      int value = it->p * 255;
      int x = it->x / 2;
      int y = it->y / 2;
      // Write into raw
      *raw.ptr(it->y, it->x) = value;

      // Check to see which quadrant it belongs in
      if (it->y % 2) {
        if (it->x % 2) {
          *bottomRight.ptr(y, x) = value;
        } else {
          *bottomLeft.ptr(y, x) = value;
        }
      } else {
        if (it->x % 2) {
          *topRight.ptr(y, x) = value;
        } else {
          *topLeft.ptr(y, x) = value;
        }
      }
    }
  };

  // Register the callback
  spdlog::info("Starting the camera stream");
  camera.cd().add_callback(callback);
  camera.start();
  spdlog::info("Successfully started camera stream");

  // Create the window
  cv::namedWindow("Raw");

  // Loop while running
  spdlog::info("Started main loop");

  while (true) {
    // Show the image
    cv::imshow("Row", raw);
    cv::imshow("top left", topLeft);
    cv::imshow("top right", topRight);
    cv::imshow("bottom left", bottomLeft);
    cv::imshow("bottom right", bottomRight);

    // Wait for user input
    char input = cv::waitKey(1);

    // Exit
    if (input == 'q') {
      spdlog::info("User termination");
      break;
    }

    // Fade
    raw /= 2;
    topLeft /= 2;
    topRight /= 2;
    bottomLeft /= 2;
    bottomRight /= 2;
  }
  spdlog::info("Exiting");

  // Shut down the camera
  spdlog::info("Shutting down the camera");
  camera.stop();
  spdlog::info("Successfully shut down the camera");
}
