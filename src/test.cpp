#include "ConcreteCameraStream.hpp"
#include "ICameraStream.hpp"
#include "IConfig.hpp"
#include "MockCameraStream.hpp"
#include "RawViewer.hpp"
#include "Registry.hpp"
#include "SubRegionViewer.hpp" // Added SubRegionViewer

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// GLFW error callback
static void glfw_error_callback(int error, const char *description) { std::cerr << "GLFW Error " << error << ": " << description << std::endl; }

int main(int, char **) {
  // Setup GLFW window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return 1;
  }

#if defined(IMGUI_IMPL_OPENGL_ES2)
  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

  GLFWwindow *window = glfwCreateWindow(1600, 1000, "Camera Stream Application", nullptr, nullptr); // Increased window size
  if (window == nullptr) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(0);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Optional

  ImGui::StyleColorsDark();

  // --- Custom Font Loading Placeholder ---
  // Your custom font loading code (FontAwesome, etc.) would go here.
  // --- End Custom Font Loading Placeholder ---

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Application-specific state
  std::shared_ptr<ICameraStream> cameraStream;
  std::shared_ptr<Registry> registry = std::make_shared<Registry>();
  std::shared_ptr<RawViewer> rawViewer;
  std::shared_ptr<SubRegionViewer> subRegionViewer; // Added SubRegionViewer instance
  bool useMockStream = false;

  // --- Initialize Camera Stream (Concrete or Mock) ---
  try {
    std::cout << "Attempting to initialize ConcreteCameraStream..." << std::endl;
    cameraStream = std::make_shared<ConcreteCameraStream>();
    std::cout << "ConcreteCameraStream initialized successfully." << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Failed to initialize ConcreteCameraStream: " << e.what() << std::endl;
    std::cout << "Falling back to MockCameraStream..." << std::endl;
    useMockStream = true;
    try {
      cameraStream = std::make_shared<MockCameraStream>(1280, 720, 3); // Mock with decent dimensions
      std::cout << "MockCameraStream initialized successfully." << std::endl;
    } catch (const std::exception &mock_e) {
      std::cerr << "Fatal: Failed to initialize MockCameraStream: " << mock_e.what() << std::endl;
      ImGui_ImplOpenGL3_Shutdown();
      ImGui_ImplGlfw_Shutdown();
      ImGui::DestroyContext();
      glfwDestroyWindow(window);
      glfwTerminate();
      return 1;
    }
  }

  // --- Setup Registry, RawViewer, and SubRegionViewer ---
  if (cameraStream) {
    std::shared_ptr<ICameraStream::Config> camConfig = cameraStream->getConfig();
    if (camConfig) {
      registry->addConfig(useMockStream ? "Mock Camera Settings" : "Camera Settings", camConfig);
    } else {
      std::cerr << "Warning: Could not get ICameraStream::Config object." << std::endl;
    }

    rawViewer = std::make_shared<RawViewer>(cameraStream, useMockStream ? "Mock Full View" : "Live Full View");

    subRegionViewer = std::make_shared<SubRegionViewer>(cameraStream, useMockStream ? "Mock Sub-Regions" : "Live Sub-Regions");
    if (subRegionViewer && subRegionViewer->getConfigUI()) {
      registry->addConfig("Sub-Region Viewer Settings", subRegionViewer->getConfigUI());
    }

  } else {
    std::cerr << "Fatal: No camera stream available after initialization attempts." << std::endl;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  bool showSettingsWindow = true;

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Settings Panel", nullptr, &showSettingsWindow);
        if (rawViewer) {
          bool rawViewerOpen = rawViewer->isOpen();
          if (ImGui::MenuItem("Full Camera Viewer", nullptr, &rawViewerOpen)) {
            rawViewer->setOpen(rawViewerOpen);
          }
        }
        if (subRegionViewer) { // Menu item for SubRegionViewer
          bool subRegionViewerOpen = subRegionViewer->isOpen();
          if (ImGui::MenuItem("Sub-Region Viewer", nullptr, &subRegionViewerOpen)) {
            subRegionViewer->setOpen(subRegionViewerOpen);
          }
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit")) {
          glfwSetWindowShouldClose(window, true);
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    if (showSettingsWindow) {
      ImGui::Begin("Application Settings", &showSettingsWindow);
      if (registry) {
        registry->renderImGui();
      }
      ImGui::Separator();
      if (cameraStream) {
        if (ImGui::Button(cameraStream->isRunning() ? "Stop Stream" : "Start Stream")) {
          try {
            if (cameraStream->isRunning()) {
              cameraStream->stop();
            } else {
              cameraStream->start(14);
            }
          } catch (const std::exception &e) {
            std::cerr << "Error starting/stopping stream from UI: " << e.what() << std::endl;
          }
        }
      }
      ImGui::End();
    }

    if (rawViewer && rawViewer->isOpen()) {
      rawViewer->render();
    }

    if (subRegionViewer && subRegionViewer->isOpen()) { // Render SubRegionViewer
      subRegionViewer->render();
    }

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(25.f / 255.f, 100.f / 255.f, 150.f / 255.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  std::cout << "Cleaning up..." << std::endl;
  if (cameraStream && cameraStream->isRunning()) {
    try {
      cameraStream->stop();
    } catch (const std::exception &e) {
      std::cerr << "Exception during final stream stop: " << e.what() << std::endl;
    }
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  std::cout << "Application terminated." << std::endl;
  return 0;
}
