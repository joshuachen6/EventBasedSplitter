#pragma once
#include <cstdint>
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
int initialize(uint8_t **buffer, uint32_t *width, uint32_t *height);
/**
 * @brief Starts the stream
 *
 * @param numThreads The number of threads to run with
 *
 * @return The status
 */
int start(uint32_t numThreads);
/**
 * @brief Stops the camera and tries to clean it up
 *
 * @return The status
 */
int stop();
/**
 * @brief Sets if the output should be verbose
 *
 * @param verbose If the output should be verbose
 * @return The status
 */
int setVerbose(bool verbose);
/**
 * @brief Sets the time it takes to fade
 *
 * @param milliseconds The time to fade in milliseconds
 * @return The status
 */
int setFadeTime(uint32_t milliseconds);
/**
 * @brief Get the fade time
 *
 * @param fadeTime The pointer to write the fade time in milliseconds to
 * @return The status
 */
int getFadeTime(uint32_t *fadeTime);
/**
 * @brief Get the timeout time
 *
 * @param timeout The event timeout time in milliseconds written into the
 * pointer
 * @return The status
 */
int getTimeout(uint32_t *timeout);
/**
 * @brief Sets the timeout
 *
 * @param timeout The timeout for events
 * @return The status
 */
int setTimeout(uint32_t timeout);
}
