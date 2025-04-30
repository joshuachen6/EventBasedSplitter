#include "ThreadPool.h"
#include "streamer.h"
#include <chrono>
#include <metavision/sdk/base/events/event_cd.h>
#include <opencv2/core/mat.hpp>
#include <ratio>
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
        // Check if there are tasks
        if (not taskQueue.empty()) {
          std::pair<const Metavision::EventCD *, const Metavision::EventCD *>
              events;
          // Remove from the queue
          auto status = taskQueue.try_pop(events);
          // Check if successful
          if (status) {
            // Process the tasks
            processEvents(events.first, events.second, i);
          }
        } else {
          auto now = std::chrono::high_resolution_clock::now();
          auto delta = startTime - now;
          auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(delta)
                        .count();

          fade(i, dt);
          startTime = std::chrono::high_resolution_clock::now();
        }
      }
    });
  }

  spdlog::debug("Created thread pool with {} threads", threads);
}

void ThreadPool::addTask(const std::pair<const Metavision::EventCD *,
                                         const Metavision::EventCD *> &task) {
  // Add to queue
  taskQueue.push(task);
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

void ThreadPool::fade(int index, long long dt) {
  uint8_t factor = (1.0 / dt) * 255;
  cv::Mat &mat = workBuffer[index];
  for (auto it = mat.begin<cv::Vec3b>(); it != mat.end<cv::Vec3b>(); ++it) {
    for (auto i = 0; i < 3; ++i) {
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

void ThreadPool::shutdown() {
  running = false;

  for (auto &thread : threadPool) {
    thread.join();
  }
}
