#pragma once

#include <condition_variable>
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
  std::mutex waitMutex;
  std::condition_variable conditionVariable;
  uint32_t fadeFrequency = 60;
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
   */
  void fade(int index);

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
   * @brief Sets the fade frequency
   *
   * @param frequency The number of times the fade is applied per second
   */
  void setFadeFrequency(uint32_t frequency);
  /**
   * @brief Gets the number of times fade is applied per second
   *
   * @return The frequency
   */
  uint32_t getFadeFrequency();
  /**
   * @brief Stops the thread pool
   */
  void shutdown();
};
