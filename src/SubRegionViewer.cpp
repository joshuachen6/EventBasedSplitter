#include "SubRegionViewer.hpp"
#include <GLFW/glfw3.h>
#include <algorithm> // For std::min/max
#include <imgui.h>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

// --- SubRegionViewer::Config Implementation ---

SubRegionViewer::Config::Config(SubRegionViewer *parent)
    : parentViewer(parent), cornerOffsetX(0), cornerOffsetY(0),
      subViewWidth(20), subViewHeight(20), subViewScale(10.0f),
      showCenterView(true), showTopLeftView(true), showTopRightView(true),
      showBottomLeftView(true), showBottomRightView(true) {
  if (!parentViewer) {
    spdlog::error("SubRegionViewer::Config: Initialized with a null parent "
                  "SubRegionViewer pointer.");
    throw std::invalid_argument(
        "Parent viewer instance cannot be null for SubRegionViewer::Config.");
  }
}

void SubRegionViewer::Config::render() {
  if (!parentViewer || !parentViewer->cameraStream) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                       "Error: Parent Viewer or CameraStream not available.");
    return;
  }

  uint32_t streamW = 0;
  uint32_t streamH = 0;
  if (parentViewer->cameraStream->isInitialized()) {
    streamW = parentViewer->cameraStream->getWidth();
    streamH = parentViewer->cameraStream->getHeight();
  }

  int maxW = streamW > 0 ? static_cast<int>(streamW) : 1920;
  int maxH = streamH > 0 ? static_cast<int>(streamH) : 1080;

  ImGui::PushID("SubRegionViewerConfig");

  ImGui::Text("Sub-View Dimensions (pixels):");
  ImGui::DragInt("ROI Width##SubViewDim", &subViewWidth, 1.0f, 10, maxW);
  ImGui::DragInt("ROI Height##SubViewDim", &subViewHeight, 1.0f, 10, maxH);
  subViewWidth = std::max(10, subViewWidth);
  subViewHeight = std::max(10, subViewHeight);

  ImGui::Text("Corner Offsets (from edge):");
  int maxOffsetX =
      streamW > 0 ? (static_cast<int>(streamW) - subViewWidth) / 2 - 5 : 200;
  int maxOffsetY =
      streamH > 0 ? (static_cast<int>(streamH) - subViewHeight) / 2 - 5 : 200;
  maxOffsetX = std::max(0, maxOffsetX);
  maxOffsetY = std::max(0, maxOffsetY);

  ImGui::DragInt("Offset X##CornerOffset", &cornerOffsetX, 1.0f, 0, maxOffsetX);
  ImGui::DragInt("Offset Y##CornerOffset", &cornerOffsetY, 1.0f, 0, maxOffsetY);
  cornerOffsetX = std::max(0, cornerOffsetX);
  cornerOffsetY = std::max(0, cornerOffsetY);

  ImGui::Text("Display Scale Factor:");
  ImGui::DragFloat("Scale##SubViewScale", &subViewScale, 0.05f, 0.1f, 10.0f,
                   "%.2f");
  subViewScale = std::max(0.1f, subViewScale);

  ImGui::Separator();
  ImGui::Text("Toggle Views:");
  ImGui::Checkbox("Show Center View", &showCenterView);
  ImGui::Checkbox("Show Top-Left View", &showTopLeftView);
  ImGui::Checkbox("Show Top-Right View", &showTopRightView);
  ImGui::Checkbox("Show Bottom-Left View", &showBottomLeftView);
  ImGui::Checkbox("Show Bottom-Right View", &showBottomRightView);

  ImGui::PopID();
}

// --- SubRegionViewer Implementation ---

SubRegionViewer::SubRegionViewer(std::shared_ptr<ICameraStream> stream,
                                 const std::string &title)
    : cameraStream(stream), windowTitle(title), showWindow(true) {
  configController = std::make_shared<Config>(this);
  subViewTextureIds.fill(0);
  subViewTextureWidths.fill(0);
  subViewTextureHeights.fill(0);

  if (!cameraStream) {
    spdlog::error("SubRegionViewer: Initialized with a null ICameraStream "
                  "pointer for title '{}'.",
                  windowTitle);
  }
  spdlog::debug("SubRegionViewer '{}' created.", windowTitle);
}

SubRegionViewer::~SubRegionViewer() {
  cleanupAllSubViewTextures();
  spdlog::debug("SubRegionViewer '{}' destroyed.", windowTitle);
}

std::shared_ptr<::IConfig> SubRegionViewer::getConfigUI() {
  return configController;
}

void SubRegionViewer::cleanupSubViewTexture(SubViewType type) {
  if (subViewTextureIds[type] != 0) {
    glDeleteTextures(1, &subViewTextureIds[type]);
    spdlog::debug(
        "SubRegionViewer '{}': OpenGL texture {} for view {} deleted.",
        windowTitle, subViewTextureIds[type], (int)type);
    subViewTextureIds[type] = 0;
    subViewTextureWidths[type] = 0;
    subViewTextureHeights[type] = 0;
  }
}

void SubRegionViewer::cleanupAllSubViewTextures() {
  for (int i = 0; i < SubViewType::COUNT; ++i) {
    cleanupSubViewTexture(static_cast<SubViewType>(i));
  }
}

bool SubRegionViewer::initializeSubViewTexture(SubViewType type, uint32_t width,
                                               uint32_t height) {
  if (width == 0 || height == 0) {
    spdlog::warn("SubRegionViewer '{}': Attempted to initialize texture for "
                 "view {} with zero dimensions.",
                 windowTitle, (int)type);
    return false;
  }

  if (subViewTextureIds[type] != 0 && (subViewTextureWidths[type] != width ||
                                       subViewTextureHeights[type] != height)) {
    cleanupSubViewTexture(type);
  }

  if (subViewTextureIds[type] == 0) {
    glGenTextures(1, &subViewTextureIds[type]);
    glBindTexture(GL_TEXTURE_2D, subViewTextureIds[type]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Important: Set unpack alignment before allocating texture data if data
    // were passed directly For nullptr data, it's less critical for allocation
    // itself but good for consistency. The actual data upload in
    // updateSubViewTexture is where it's most important. GLint
    // previousAlignment; glGetIntegerv(GL_UNPACK_ALIGNMENT,
    // &previousAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Expect tightly packed data

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR,
                 GL_UNSIGNED_BYTE, nullptr);

    // glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment); // Restore
    // previous alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT,
                  4); // Restore default alignment (more common)

    subViewTextureWidths[type] = width;
    subViewTextureHeights[type] = height;
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  return subViewTextureIds[type] != 0;
}

void SubRegionViewer::updateSubViewTexture(SubViewType type,
                                           const cv::Mat &subImage) {
  if (subImage.empty() || subImage.channels() != 3 ||
      subImage.type() != CV_8UC3) {
    return;
  }
  uint32_t subWidth = subImage.cols;
  uint32_t subHeight = subImage.rows;

  if (!initializeSubViewTexture(
          type, subWidth,
          subHeight)) { // Initializes or reinitializes if needed
    return;
  }

  glBindTexture(GL_TEXTURE_2D, subViewTextureIds[type]);

  // GLint previousAlignment;
  // glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
  glPixelStorei(
      GL_UNPACK_ALIGNMENT,
      1); // Tell OpenGL that data is tightly packed (1-byte alignment)

  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, subWidth, subHeight, GL_BGR,
                  GL_UNSIGNED_BYTE, subImage.data);

  // glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment); // Restore previous
  // alignment
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Restore default alignment

  glBindTexture(GL_TEXTURE_2D, 0);
}

void SubRegionViewer::render() {
  if (!showWindow)
    return;

  ImGui::Begin(windowTitle.c_str(), &showWindow);

  if (!cameraStream || !cameraStream->isInitialized()) {
    ImGui::Text("Camera stream not available or not initialized.");
    ImGui::End();
    return;
  }

  if (!cameraStream->isRunning()) {
    ImGui::Text(
        "Camera stream is not running. Start the stream to view sub-regions.");
    ImGui::End();
    return;
  }

  uint8_t *fullBuffer = cameraStream->getBuffer();
  uint32_t fullWidth = cameraStream->getWidth();
  uint32_t fullHeight = cameraStream->getHeight();

  if (!fullBuffer || fullWidth == 0 || fullHeight == 0) {
    ImGui::Text("Waiting for valid stream data...");
    ImGui::End();
    return;
  }

  cv::Mat fullFrameMat(fullHeight, fullWidth, CV_8UC3, fullBuffer);

  int roiW = configController->subViewWidth;
  int roiH = configController->subViewHeight;
  int offX = configController->cornerOffsetX;
  int offY = configController->cornerOffsetY;
  float scale = configController->subViewScale;

  roiW = std::min(std::max(10, roiW), (int)fullWidth);
  roiH = std::min(std::max(10, roiH), (int)fullHeight);
  // Ensure offsets combined with ROI dimensions do not exceed full frame
  // dimensions
  offX = std::min(std::max(0, offX), (int)fullWidth - roiW);
  offY = std::min(std::max(0, offY), (int)fullHeight - roiH);

  ImVec2 scaledDisplaySize(static_cast<float>(roiW) * scale,
                           static_cast<float>(roiH) * scale);

  auto renderViewWithLabel = [&](SubViewType type, const char *label,
                                 cv::Rect roi) {
    // Ensure ROI is valid before trying to create a sub-image
    if (roi.x < 0 || roi.y < 0 || roi.width <= 0 || roi.height <= 0 ||
        roi.x + roi.width > (int)fullWidth ||
        roi.y + roi.height > (int)fullHeight) {
      // Optionally log this invalid ROI attempt
      // spdlog::warn("SubRegionViewer: ROI for {} ({},{},{}x{}) is out of
      // bounds for full {}x{}.",
      //              label, roi.x, roi.y, roi.width, roi.height, fullWidth,
      //              fullHeight);
      ImGui::BeginGroup();
      ImGui::Text("%s: Invalid ROI", label);
      ImGui::Dummy(scaledDisplaySize); // Placeholder for size
      ImGui::EndGroup();
      return;
    }
    cv::Mat subImage = fullFrameMat(roi);
    updateSubViewTexture(type, subImage);
    if (subViewTextureIds[type] != 0) {
      ImGui::BeginGroup();
      ImGui::Text("%s", label);
      ImGui::Image(static_cast<ImTextureID>(subViewTextureIds[type]),
                   scaledDisplaySize);
      ImGui::EndGroup();
    }
  };

  float windowContentWidth = ImGui::GetContentRegionAvail().x;
  float itemSpacing = ImGui::GetStyle().ItemSpacing.x;

  // Top Row
  if (configController->showTopLeftView) {
    cv::Rect tlRoi(offX, offY, roiW, roiH);
    renderViewWithLabel(SubViewType::TOP_LEFT, "Top-Left", tlRoi);
  }

  if (configController->showTopRightView) {
    float trPosX = windowContentWidth - scaledDisplaySize.x -
                   ImGui::GetStyle().WindowPadding.x; // Adjust for padding
    trPosX = std::max(0.0f, trPosX);                  // Ensure not negative
    if (configController->showTopLeftView &&
        subViewTextureIds[SubViewType::TOP_LEFT] != 0) {
      float currentXAfterTL =
          ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x + itemSpacing;
      trPosX = std::max(currentXAfterTL, trPosX);
      ImGui::SameLine(
          0, 0); // Stay on same line, but allow custom pos if needed after this
      ImGui::SetCursorPosX(trPosX);
    } else {
      ImGui::SetCursorPosX(trPosX);
    }
    cv::Rect trRoi(fullWidth - roiW - offX, offY, roiW, roiH);
    renderViewWithLabel(SubViewType::TOP_RIGHT, "Top-Right", trRoi);
  }
  ImGui::NewLine();

  // Center Row
  if (configController->showCenterView) {
    float centerPosX = (windowContentWidth - scaledDisplaySize.x) * 0.5f;
    centerPosX = std::max(ImGui::GetStyle().WindowPadding.x, centerPosX);
    ImGui::SetCursorPosX(centerPosX);
    cv::Rect centerRoi((fullWidth - roiW) / 2, (fullHeight - roiH) / 2, roiW,
                       roiH);
    renderViewWithLabel(SubViewType::CENTER, "Center", centerRoi);
  }
  ImGui::NewLine();

  // Bottom Row
  if (configController->showBottomLeftView) {
    cv::Rect blRoi(offX, fullHeight - roiH - offY, roiW, roiH);
    renderViewWithLabel(SubViewType::BOTTOM_LEFT, "Bottom-Left", blRoi);
  }

  if (configController->showBottomRightView) {
    float brPosX = windowContentWidth - scaledDisplaySize.x -
                   ImGui::GetStyle().WindowPadding.x; // Adjust for padding
    brPosX = std::max(0.0f, brPosX);                  // Ensure not negative
    if (configController->showBottomLeftView &&
        subViewTextureIds[SubViewType::BOTTOM_LEFT] != 0) {
      float currentXAfterBL =
          ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x + itemSpacing;
      brPosX = std::max(currentXAfterBL, brPosX);
      ImGui::SameLine(0, 0);
      ImGui::SetCursorPosX(brPosX);
    } else {
      ImGui::SetCursorPosX(brPosX);
    }
    cv::Rect brRoi(fullWidth - roiW - offX, fullHeight - roiH - offY, roiW,
                   roiH);
    renderViewWithLabel(SubViewType::BOTTOM_RIGHT, "Bottom-Right", brRoi);
  }

  ImGui::End();
}

void SubRegionViewer::setOpen(bool open) { showWindow = open; }

bool SubRegionViewer::isOpen() const { return showWindow; }
