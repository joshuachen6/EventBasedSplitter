#pragma once

#include "ICameraStream.hpp" // To get camera data
#include "IConfig.hpp"       // For its own nested Config
#include <array>             // For storing multiple texture IDs
#include <cstdint>           // For uint32_t, uint8_t
#include <memory>            // For std::shared_ptr
#include <string>
#include <vector>

// Forward declare GLuint
typedef unsigned int GLuint;

// Forward declare cv::Mat to avoid including OpenCV in this header
namespace cv {
class Mat;
}

/**
 * @brief Displays multiple sub-regions (center, corners) from an ICameraStream using ImGui.
 * Allows configuration of sub-view dimensions, offsets, and scaling.
 */
class SubRegionViewer {
public:
  /**
   * @brief Nested concrete class for SubRegionViewer configuration.
   */
  class Config : public ::IConfig {
  public:
    explicit Config(SubRegionViewer *parent);
    void render() override;

    // Configurable properties
    int cornerOffsetX;
    int cornerOffsetY;
    int subViewWidth;
    int subViewHeight;
    float subViewScale;
    bool showCenterView;
    bool showTopLeftView;
    bool showTopRightView;
    bool showBottomLeftView;
    bool showBottomRightView;

  private:
    SubRegionViewer *parentViewer; // Pointer to the outer class
  };

  /**
   * @brief Constructor.
   * @param stream A shared pointer to the ICameraStream instance to be viewed.
   * @param title The title for the ImGui window of this viewer.
   */
  SubRegionViewer(std::shared_ptr<ICameraStream> stream, const std::string &title = "Sub-Region Viewer");

  /**
   * @brief Destructor.
   * Cleans up OpenGL textures.
   */
  ~SubRegionViewer();

  // Prevent copying and moving.
  SubRegionViewer(const SubRegionViewer &) = delete;
  SubRegionViewer &operator=(const SubRegionViewer &) = delete;
  SubRegionViewer(SubRegionViewer &&) = delete;
  SubRegionViewer &operator=(SubRegionViewer &&) = delete;

  /**
   * @brief Renders the sub-region views within its own ImGui window.
   * This should be called every frame within your ImGui rendering loop.
   */
  void render();

  /**
   * @brief Gets a shared pointer to the configuration object for this viewer.
   * @return A shared_ptr to the SubRegionViewer::Config object (cast to ::IConfig for Registry).
   */
  std::shared_ptr<::IConfig> getConfigUI();

  void setOpen(bool open);
  bool isOpen() const;

private:
  friend class Config; // Allow Config to access parentViewer members if needed, though it primarily modifies its own state

  enum SubViewType {
    CENTER = 0,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    COUNT // Number of sub-view types
  };

  std::shared_ptr<ICameraStream> cameraStream;
  std::string windowTitle;
  bool showWindow;

  std::array<GLuint, SubViewType::COUNT> subViewTextureIds;
  // Store original dimensions of data uploaded to texture to avoid re-upload if sub-view content doesn't change dimensions
  std::array<uint32_t, SubViewType::COUNT> subViewTextureWidths;
  std::array<uint32_t, SubViewType::COUNT> subViewTextureHeights;

  std::shared_ptr<Config> configController; // Instance of the nested config

  bool initializeSubViewTexture(SubViewType type, uint32_t width, uint32_t height);
  void updateSubViewTexture(SubViewType type, const cv::Mat &subImage);
  void cleanupSubViewTexture(SubViewType type);
  void cleanupAllSubViewTextures();
};
