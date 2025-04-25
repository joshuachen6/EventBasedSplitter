#pragma once
#include <metavision/sdk/stream/camera.h>

extern "C" {
/**
 * @brief Starts the stream
 *
 * @param rawBuffer The raw buffer to stream into
 * @param topLeftBuffer The top left buffer
 * @param topRightBuffer The top right buffer
 * @param bottomLeftBuffer The bottom left buffer
 * @param bottomRightBuffer The bottom right buffer
 */
void init(uint8_t *rawBuffer, uint8_t *topLeftBuffer, uint8_t *topRightBuffer,
          uint8_t *bottomLeftBuffer, uint8_t *bottomRightBuffer);
/**
 * @brief Sets the offset for the corner viewers
 *
 * @param offset The offset in pixels
 */
void setROIOffset(uint8_t offset);
/**
 * @brief Sets the scale of the corner viewers
 *
 * @param scale The scale
 */
void setROIScale(uint8_t scale);
/**
 * @brief Sets the size of the corner viewers
 *
 * @param size The size in pixels
 */
void setROISize(uint8_t size);
/**
 * @brief Starts the streaming
 */
void start();
/**
 * @brief Stops the streaming and cleans up
 */
void stop();
}
