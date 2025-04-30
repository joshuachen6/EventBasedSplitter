#pragma once

#include <metavision/sdk/base/events/event_cd.h>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <tbb/concurrent_queue.h>
#include <thread>
#include <utility>

class ThreadPool {
private:
  tbb::concurrent_queue<
      std::pair<const Metavision::EventCD *, const Metavision::EventCD *>>
      taskQueue;
  std::vector<std::thread> threadPool;
  std::vector<cv::Mat> workBuffer;
  bool running;

  uint32_t fadeTime = 1e3;

  /**
   * @brief Processes the events
   *
   * @param begin The start iterator
   * @param end The end iterator
   * @param bufferIndex The work buffer to use
   */
  void processEvents(const Metavision::EventCD *begin,
                     const Metavision::EventCD *end, int bufferIndex);
  /**
   * @brief Fades a given mat
   *
   * @param index The mat to fade
   * @param dt Change in time
   */
  void fade(int index, long long dt);

public:
  /**
   * @brief Creates a thread pool with a given number of threads
   *
   * @param threads The number of threads
   * @param imageSize The size of the image
   */
  ThreadPool(int threads, cv::Size imageSize);
  /**
   * @brief Processes the events
   *
   * @param task The events to be processed
   */
  void addTask(const std::pair<const Metavision::EventCD *,
                               const Metavision::EventCD *> &task);
  /**
   * @brief Sums the frames together
   *
   * @param output The output buffer to write to
   */
  void sum(cv::Mat &output);
  /**
   * @brief Sets the time to fade
   *
   * @param milliseconds The time to fade in milliseconds
   */
  void setFadeTime(uint32_t milliseconds);
  /**
   * @brief Gets the time it takes to fade
   *
   * @return The fade time in milliseconds
   */
  uint32_t getFadeTime();
  /**
   * @brief Stops the thread pool
   */
  void shutdown();
};
