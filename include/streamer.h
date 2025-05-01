#pragma once
#include <cstdint>

extern "C" {
typedef struct StreamerInstance StreamerInstance;

/**
 * @brief Sets up the camera stream
 *
 * @param Instance The address to store the stream instance pointer
 * @param buffer The address to store the camera stream pointer
 * @param width The address to store the width of the camera
 * @param height The address to store the height of the camera
 *
 * @return The status
 */
int initialize(StreamerInstance **instance, uint8_t **buffer, uint32_t *width, uint32_t *height);
/**
 * @brief Starts the stream
 *
 * @param Instance The stream instance pointer
 * @param numThreads The number of threads to run with
 *
 * @return The status
 */
int start(StreamerInstance *instance, uint32_t numThreads);
/**
 * @brief Stops the camera and tries to clean it up
 *
 * @param Instance The stream instance pointer
 *
 * @return The status
 */
int stop(StreamerInstance *instance);
/**
 * @brief Sets if the output should be verbose
 *
 * @param Instance The stream instance pointer
 * @param verbose If the output should be verbose
 *
 * @return The status
 */
int setVerbose(StreamerInstance *instance, uint8_t verbose);
/**
 * @brief Sets the time it takes to fade
 *
 * @param Instance The stream instance pointer
 * @param milliseconds The time to fade in milliseconds
 *
 * @return The status
 */
int setFadeTime(StreamerInstance *instance, uint32_t milliseconds);
/**
 * @brief Get the fade time
 *
 * @param Instance The stream instance pointer
 * @param fadeTime The pointer to write the fade time in milliseconds to
 *
 * @return The status
 */
int getFadeTime(StreamerInstance *instance, uint32_t *fadeTime);
}
