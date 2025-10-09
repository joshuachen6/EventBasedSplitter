#include "RawViewer.hpp"
#include <imgui.h>
#include <spdlog/spdlog.h>

// OpenGL function declarations will typically come from the system OpenGL
// headers, which are often included by <GLFW/glfw3.h>. The
// ImGui_ImplOpenGL3_Init() call in main.cpp loads the actual function pointers.
#include <GLFW/glfw3.h> // Make sure this provides GL types like GLuint and function declarations

RawViewer::RawViewer(std::shared_ptr<ICameraStream> stream,
                     const std::string &title)
    : cameraStream(stream), windowTitle(title), showWindow(true),
      openglTextureId(0), textureWidth(0), textureHeight(0) {
  if (!cameraStream) {
    spdlog::error("RawViewer: Initialized with a null ICameraStream pointer "
                  "for title '{}'.",
                  windowTitle);
    return;
  }
  spdlog::debug("RawViewer '{}' created.", windowTitle);
}

RawViewer::~RawViewer() {
  cleanupTexture();
  spdlog::debug("RawViewer '{}' destroyed.", windowTitle);
}

void RawViewer::cleanupTexture() {
  if (openglTextureId != 0) {
    // Ensure an OpenGL context is current if this destructor can be called
    // after the main context is destroyed, though typically it's before.
    glDeleteTextures(1, &openglTextureId);
    spdlog::debug("RawViewer '{}': OpenGL texture {} deleted.", windowTitle,
                  openglTextureId);
    openglTextureId = 0;
    textureWidth = 0;
    textureHeight = 0;
  }
}

bool RawViewer::initializeTexture(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0) {
    spdlog::error(
        "RawViewer '{}': Attempted to initialize texture with zero dimensions.",
        windowTitle);
    return false;
  }

  if (openglTextureId != 0 &&
      (textureWidth != width || textureHeight != height)) {
    cleanupTexture(); // Delete old texture if dimensions changed
  }

  if (openglTextureId == 0) {
    glGenTextures(1, &openglTextureId);
    glBindTexture(GL_TEXTURE_2D, openglTextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Assuming BGR, 3 channels from cameraStream->getBuffer()
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR,
                 GL_UNSIGNED_BYTE, nullptr);

    textureWidth = width;
    textureHeight = height;
    spdlog::debug("RawViewer '{}': OpenGL texture {} initialized ({}x{}).",
                  windowTitle, openglTextureId, width, height);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  return openglTextureId != 0;
}

void RawViewer::updateTexture() {
  if (!cameraStream || !cameraStream->isRunning() ||
      !cameraStream->isInitialized()) {
    return;
  }

  uint32_t currentWidth = cameraStream->getWidth();
  uint32_t currentHeight = cameraStream->getHeight();
  uint8_t *buffer = cameraStream->getBuffer();

  if (buffer == nullptr || currentWidth == 0 || currentHeight == 0) {
    return;
  }

  if (openglTextureId == 0 || textureWidth != currentWidth ||
      textureHeight != currentHeight) {
    if (!initializeTexture(currentWidth, currentHeight)) {
      spdlog::error("RawViewer '{}': Failed to initialize texture for update.",
                    windowTitle);
      return;
    }
  }

  glBindTexture(GL_TEXTURE_2D, openglTextureId);
  // Assuming buffer format is BGR, 3 channels, uint8_t per component.
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_BGR,
                  GL_UNSIGNED_BYTE, buffer);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void RawViewer::render() {
  if (!showWindow) {
    return;
  }

  ImGui::Begin(windowTitle.c_str(), &showWindow);

  if (!cameraStream) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                       "Error: ICameraStream is not available.");
    ImGui::End();
    return;
  }

  if (!cameraStream->isInitialized()) {
    ImGui::Text("Camera stream is not initialized.");
    ImGui::End();
    return;
  }

  // Update texture only if the stream is running.
  // If stopped, show the last frame or a placeholder.
  if (cameraStream->isRunning()) {
    updateTexture();
  }

  if (openglTextureId != 0 && textureWidth > 0 && textureHeight > 0) {
    // Corrected cast for ImTextureID if it's an integer type like unsigned long
    // long
    ImGui::Image(
        static_cast<ImTextureID>(static_cast<intptr_t>(openglTextureId)),
        ImVec2(static_cast<float>(textureWidth),
               static_cast<float>(textureHeight)));
  } else if (cameraStream->isRunning()) { // If running but texture not ready
    ImGui::Text("Waiting for stream data or texture initialization...");
  } else { // If not running and no texture (or to show a message)
    ImGui::Text("Camera stream is not running. Start the stream to view.");
  }

  ImGui::End();
}

void RawViewer::setOpen(bool open) { showWindow = open; }

bool RawViewer::isOpen() const { return showWindow; }
