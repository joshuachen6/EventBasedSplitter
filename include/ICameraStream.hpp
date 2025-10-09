#pragma once

#include "IConfig.hpp" // The global IConfig interface
#include <cstdint>
#include <memory> // For std::shared_ptr, std::make_shared
#include <string>

// Forward declare ImGui for Config class members if needed, or include imgui.h in ICameraStream.cpp
// struct ImVec4;

/**
 * @brief Abstract interface for a camera stream.
 * Defines the common operations for camera stream implementations.
 * Also defines a nested concrete class for its configuration UI.
 */
class ICameraStream {
public:
  /**
   * @brief Concrete nested class for ICameraStream configuration.
   * This class is responsible for rendering ImGui elements
   * to control its parent ICameraStream instance.
   */
  class Config : public ::IConfig {
  public:
    /**
     * @brief Constructor for the Config UI.
     * @param parentStream Pointer to the ICameraStream instance this config will control.
     */
    explicit Config(ICameraStream *parentStream);
    ~Config() override = default;

    void render() override;

  private:
    ICameraStream *parentStreamInstance; // Pointer to the outer class instance
    // UI state variables
    bool uiVerbose;
    int uiFadeTimeMs;      // Using int for ImGui::DragInt
    int uiFadeFrequencyHz; // Using int for ImGui::DragInt

    void loadInitialValues();   // Helper to load values from parentStreamInstance
    friend class ICameraStream; // Allow ICameraStream to manage its Config state if needed
  };

  virtual ~ICameraStream() = default;

  /**
   * @brief Gets a shared pointer to the configuration object for this stream.
   * @return A shared_ptr to the ICameraStream::Config object.
   */
  virtual std::shared_ptr<ICameraStream::Config> getConfig() = 0;

  // Stream operation methods
  virtual void start(uint32_t numThreads) = 0;
  virtual void stop() = 0;
  virtual void setVerbose(bool verbose) = 0;
  // Need a way for Config to get the current verbose state if it's to display it.
  // Option 1: Add getVerbose() to ICameraStream
  // Option 2: Concrete streams store it and Config accesses it (less clean for interface)
  // Let's assume for now Config's uiVerbose is a send-only control, or add getVerbose.
  // For a better UI, getVerbose would be good.
  virtual bool getVerboseState() const = 0; // Added for Config to read current state

  virtual void setFadeTime(uint32_t milliseconds) = 0;
  virtual uint32_t getFadeTime() = 0;
  virtual void setFadeFrequency(uint32_t frequency) = 0;
  virtual uint32_t getFadeFrequency() = 0;
  virtual uint8_t *getBuffer() const = 0;
  virtual uint32_t getWidth() const = 0;
  virtual uint32_t getHeight() const = 0;
  virtual bool isInitialized() const = 0;
  virtual bool isRunning() const = 0;

protected:
  // Protected constructor for base class if needed, though not strictly necessary for pure interface
  ICameraStream() = default;
};
