#pragma once
#include <metavision/sdk/stream/camera.h>

extern "C" {
  /**
   * @brief Sets up the camera stream
   *
   * @param buffer The address to store the camera stream pointer
   * @param width The address to store the width of the camera
   * @param height The address to store the height of the camera
   *
   * @return The status
   */
  int initialize(uint8_t **buffer, uint8_t *width, uint8_t *height);
  /**
   * @brief Starts the stream
   *
   * @return The status
   */
  int start();
  /**
   * @brief Stops the camera and tries to clean it up
   *
   * @return The status
   */
  int stop();
}
