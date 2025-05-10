#include "ThreadPool.h"
#include <chrono>
#include <metavision/sdk/base/events/event_cd.h>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <spdlog/spdlog.h>

ThreadPool::ThreadPool(int threads, cv::Size imageSize) {
  // Reserve space
  threadPool.reserve(threads);
  workBuffer.resize(threads);

  // Fill buffers
  std::fill(workBuffer.begin(), workBuffer.end(),
            cv::Mat::zeros(imageSize, CV_8UC3));

  // Start running
  running = true;

  // Create threads
  for (auto i = 0; i < threads; ++i) {
    // Add thread
    threadPool.emplace_back([&, i]() {
      auto startTime = std::chrono::high_resolution_clock::now();
      while (running) {
        // Status of the wait
        // Wait for notify
        auto now = std::chrono::high_resolution_clock::now();
        {
          // Time for when to run other task
          auto until = now + std::chrono::milliseconds(long(1e3 / 60));
          // Lock the mutex
          std::unique_lock lock(waitMutex);
          conditionVariable.wait_until(lock, until);
        }

        // Check if there are tasks
        std::pair<const Metavision::EventCD *, const Metavision::EventCD *>
            events;
        // Remove from the queue
        auto status = taskQueue.try_pop(events);
        // Check if successful
        if (status) {
          // Process the tasks
          processEvents(events.first, events.second, i);
        }
        // If not successful or if timed out
        // Fade
        auto delta = now - startTime;
        auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(delta)
                      .count();
        if (dt > 1000 / fadeFrequency) {
          fade(i);
          startTime = std::chrono::high_resolution_clock::now();
        }
      }
    });
  }

  spdlog::debug("Created thread pool with {} threads", threads);
}

void ThreadPool::addTask(const std::pair<const Metavision::EventCD *,
                                         const Metavision::EventCD *> &task) {
  // Block for the lock
  {
    // Lock the mutex
    std::lock_guard lock(waitMutex);
    // Add the task
    taskQueue.push(task);
  }
  // Wake a thread
  conditionVariable.notify_one();
}

void ThreadPool::processEvents(const Metavision::EventCD *begin,
                               const Metavision::EventCD *end,
                               int bufferIndex) {
  // Get the respective image
  cv::Mat &image = workBuffer[bufferIndex];

  // Loop through all events
  for (auto it = begin; it != end; ++it) {
    // Write into raw
    auto *ptr = image.ptr<cv::Vec3b>(it->y, it->x);
    (*ptr)[0] = it->p * 255;
    (*ptr)[1] = (1 - it->p) * 255;
  }
}

void ThreadPool::fade(int index) {
  // Calculate the factor
  uint8_t factor = 255 / (fadeTime / 1e3 / 60);
  cv::Mat &mat = workBuffer[index];
  for (auto it = mat.begin<cv::Vec3b>(); it != mat.end<cv::Vec3b>(); ++it) {
    for (auto i = 0; i < 2; ++i) {
      auto &val = (*it)[i];
      val = std::max(val - factor, 0);
    }
  }
}

void ThreadPool::sum(cv::Mat &output) {
  if (workBuffer.size()) {
    // Override output with first buffer
    workBuffer[0].copyTo(output);
    // Go through all buffers
    for (auto i = 1; i < workBuffer.size(); ++i) {
      // Combine all other buffers ontop
      output += workBuffer[i];
    }
  }
}

void ThreadPool::setFadeTime(uint32_t milliseconds) { fadeTime = milliseconds; }

uint32_t ThreadPool::getFadeTime() { return fadeTime; }

void ThreadPool::setFadeFrequency(uint32_t frequency) {
  fadeFrequency = frequency;
}

uint32_t ThreadPool::getFadeFrequency() { return fadeFrequency; }

void ThreadPool::shutdown() {
  running = false;

  for (auto &thread : threadPool) {
    thread.join();
  }
}
