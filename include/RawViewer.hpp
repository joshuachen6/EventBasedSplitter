#pragma once

#include "ICameraStream.hpp" // To get camera data
#include <cstdint>           // For uint32_t, uint8_t
#include <memory>            // For std::shared_ptr
#include <string>

// Forward declaration for GLuint to avoid including OpenGL headers here if not strictly necessary
// However, for GLuint, it's common and small enough.
// If your project uses glad or a specific loader, ensure it's included before this in .cpp
typedef unsigned int GLuint;

/**
 * @brief A component to display the raw output of an ICameraStream using ImGui.
 * It manages an OpenGL texture to render the camera frames.
 */
class RawViewer {
public:
  /**
   * @brief Constructor.
   * @param stream A shared pointer to the ICameraStream instance to be viewed.
   * @param title The title to display for this viewer instance (e.g., in an ImGui window).
   */
  RawViewer(std::shared_ptr<ICameraStream> stream, const std::string &title = "Raw Camera View");

  /**
   * @brief Destructor.
   * Cleans up the OpenGL texture.
   */
  ~RawViewer();

  // Prevent copying and moving as it manages an OpenGL resource.
  RawViewer(const RawViewer &) = delete;
  RawViewer &operator=(const RawViewer &) = delete;
  RawViewer(RawViewer &&) = delete;
  RawViewer &operator=(RawViewer &&) = delete;

  /**
   * @brief Renders the camera view within the current ImGui context.
   * This should be called every frame within your ImGui rendering loop.
   * It typically creates its own ImGui window to display the feed.
   */
  void render();

  /**
   * @brief Sets a flag to control whether the ImGui window for this viewer is open.
   * @param open True to show the window, false to hide it.
   */
  void setOpen(bool open);

  /**
   * @brief Checks if the ImGui window for this viewer is set to be open.
   * @return True if the window should be shown, false otherwise.
   */
  bool isOpen() const;

private:
  std::shared_ptr<ICameraStream> cameraStream;
  std::string windowTitle;
  bool showWindow; // To control visibility of the ImGui window

  GLuint openglTextureId;
  uint32_t textureWidth;
  uint32_t textureHeight;

  /**
   * @brief Initializes the OpenGL texture used for rendering.
   * Called once or if stream dimensions change.
   * @param width The width of the texture.
   * @param height The height of the texture.
   * @return True if texture initialization was successful, false otherwise.
   */
  bool initializeTexture(uint32_t width, uint32_t height);

  /**
   * @brief Updates the OpenGL texture with the latest frame from the cameraStream.
   */
  void updateTexture();

  /**
   * @brief Cleans up the OpenGL texture.
   */
  void cleanupTexture();
};
