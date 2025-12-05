# AI Agent Guidelines - Event Based Splitter

## IMPORTANT: READ BEFORE ANY MODIFICATIONS

This document is for **ALL AI AGENTS** working on this codebase. Follow these guidelines strictly.

## DO NOT CREATE
- **NO MORE README FILES** - One README.md exists and is sufficient
- **NO CHANGELOG FILES** - Do not create CHANGELOG.md or similar
- **NO DUPLICATE DOCUMENTATION** - Check existing files before creating new docs
- **NO TEST FILES** - Unless specifically requested, don't leave test files in the project
- **NO REDUNDANT FILES** - Always use existing entry points and modules

## PROJECT RULES

### File Management
1. **Single Entry Point**: `src/test.cpp` contains `main()` - do not create alternative entry points
2. **Use Existing Structure**: Do not reorganize directories without explicit permission
3. **Clean Up After Testing**: Remove any test files created during development
4. **Preserve User Data**: Never modify or delete user configuration or data files

### Code Standards
1. **C++20 Standard**: Use modern C++ features (concepts, ranges, etc. where appropriate)
2. **Naming Conventions**: Use PascalCase for classes, camelCase for methods/variables
3. **Interface Pattern**: All camera streams must implement `ICameraStream`
4. **Config Pattern**: All configurable components must implement `IConfig`
5. **Thread Safety**: Use mutex/atomic for shared state, TBB concurrent containers where applicable
6. **Logging**: Use spdlog for all logging (debug, info, error levels)

### Documentation Updates
1. **Update Existing Docs**: Modify existing files rather than creating new ones
2. **agent.md is Primary**: This file is the source of truth for development
3. **Maintain Consistency**: Keep all documentation synchronized

## PROJECT STRUCTURE (FIXED - DO NOT CHANGE)

```
EventBasedSplitter/
├── CMakeLists.txt          # Build configuration - DO NOT rename project
├── README.md               # User documentation - DO NOT DUPLICATE
├── agent.md                # THIS FILE - AI Agent guidelines
├── .gitignore              # Git exclusions
├── .gitmodules             # Git submodules (imgui)
├── imgui/                  # ImGui submodule (docking branch) - DO NOT MODIFY
├── build/                  # Build output directory
├── include/                # Header files
│   ├── ICameraStream.hpp   # Abstract camera stream interface
│   ├── IConfig.hpp         # Configuration interface
│   ├── BiasConfig.hpp      # Camera bias configuration UI
│   ├── ConcreteCameraStream.hpp  # Real Metavision camera
│   ├── MockCameraStream.hpp      # Mock/test camera
│   ├── RawViewer.hpp       # Full frame viewer
│   ├── SubRegionViewer.hpp # ROI sub-region viewer
│   ├── Registry.hpp        # Configuration registry
│   └── ThreadPool.h        # Parallel event processing
└── src/                    # Implementation files
    ├── test.cpp            # MAIN ENTRY POINT - contains main()
    ├── ICameraStream.cpp   # Interface implementation
    ├── BiasConfig.cpp      # Bias configuration implementation
    ├── ConcreteCameraStream.cpp  # Real camera implementation
    ├── MockCameraStream.cpp      # Mock camera implementation
    ├── RawViewer.cpp       # Viewer implementation
    ├── SubRegionViewer.cpp # Sub-region viewer implementation
    ├── Registry.cpp        # Registry implementation
    └── ThreadPool.cpp      # Thread pool implementation
```

## TECHNICAL SPECIFICATIONS

### Core Requirements
- **C++ Standard**: C++20 required
- **Build System**: CMake 3.25+
- **GUI Framework**: ImGui (docking branch) with GLFW + OpenGL3 backend
- **Camera SDK**: Metavision SDK (core, stream, ui)
- **Image Processing**: OpenCV
- **Threading**: Intel TBB, pthreads
- **Logging**: spdlog
- **Window Size**: 1600x1000 pixels (default)
- **OpenGL**: 3.2+ (3.0 on Linux)

### Dependencies (from CMakeLists.txt)
```cmake
MetavisionSDK::core
MetavisionSDK::stream
MetavisionSDK::ui
OpenCV
spdlog::spdlog
TBB::tbb
glfw
OpenGL::GL
```

### Default Parameters
- **Thread Pool Size**: 14 threads
- **Mock Resolution**: 1280x720x3 (RGB)
- **Fade Time**: 1000ms (0-60000ms range)
- **Fade Frequency**: 60Hz (1-100Hz range)
- **Sub-Region ROI**: 20x20 pixels
- **Sub-Region Scale**: 10x

## ARCHITECTURE

### Design Patterns Used
1. **Strategy Pattern**: `ICameraStream` with `ConcreteCameraStream` and `MockCameraStream`
2. **Registry Pattern**: Centralized configuration via `Registry` class
3. **Interface Pattern**: `IConfig` for all configurable components
4. **Thread Pool Pattern**: Parallel event processing with work distribution

### Data Flow
```
Camera/Mock → Events → ThreadPool (N threads) → Faded Images →
Buffer → Viewers → OpenGL Textures → ImGui → Display
```

### Threading Model
- **Main Thread**: ImGui rendering, GLFW event loop
- **Camera Thread**: Metavision event callbacks (implicit)
- **Worker Threads**: ThreadPool for parallel event processing (N threads)
- **Mock Thread**: Synthetic data generation (~30fps)

## KEY INTERFACES

### ICameraStream (Abstract Base)
```cpp
class ICameraStream {
public:
    virtual void start(uint32_t numThreads) = 0;
    virtual void stop() = 0;
    virtual void setVerbose(bool verbose) = 0;
    virtual bool getVerboseState() const = 0;
    virtual void setFadeTime(uint32_t ms) = 0;
    virtual uint32_t getFadeTime() const = 0;
    virtual void setFadeFrequency(uint32_t hz) = 0;
    virtual uint32_t getFadeFrequency() const = 0;
    virtual uint8_t* getBuffer() = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual bool isInitialized() const = 0;
    virtual bool isRunning() const = 0;
};
```

### IConfig (Configuration Interface)
```cpp
class IConfig {
public:
    virtual void render() = 0;  // Render ImGui configuration UI
};
```

### Registry Usage
```cpp
// Register a config
Registry::instance().registerConfig("Camera Settings", cameraStream->getConfig());

// Render all configs
Registry::instance().renderImGui();
```

## GUI COMPONENTS

### Menu Bar
- **View Menu**: Settings Panel, Full Camera Viewer, Sub-Region Viewer toggles
- **File Menu**: Exit

### Application Settings Panel
- Camera stream configuration (verbose, fade time, fade frequency)
- **Bias Settings** (real camera only): Configure sensor biases
- Sub-region viewer settings (ROI size, offsets, scale)
- Start/Stop stream button

### Viewers
- **RawViewer**: Full camera frame display
- **SubRegionViewer**: 5 ROI regions (center + 4 corners)

## KEY FEATURES (IMPLEMENTED)

- Real-time event-based camera capture via Metavision SDK
- Mock camera fallback for development/testing
- Multi-threaded parallel event processing
- Configurable fade effects (time and frequency)
- **Camera bias configuration** (real camera only)
- ImGui-based interactive configuration
- Full-frame and sub-region visualization
- OpenGL texture-based rendering
- Comprehensive spdlog logging

## CAMERA BIAS CONFIGURATION

The BiasConfig component provides UI controls for Metavision camera sensor biases.
Only available when using a real camera (not mock mode).

### Bias Parameters

| Bias Name | Default | Min | Max | Description |
|-----------|---------|-----|-----|-------------|
| bias_diff | 0 | -25 | 23 | Base differential bias (recommended to not change) |
| bias_diff_on | 0 | -85 | 140 | ON event contrast threshold |
| bias_diff_off | 0 | -35 | 190 | OFF event contrast threshold |
| bias_fo | 0 | -35 | 55 | Low-pass filter (high-frequency fluctuations) |
| bias_hpf | 0 | 0 | 120 | High-pass filter (background noise) |
| bias_refr | 0 | -20 | 235 | Refractory period after each event |

### Tuning Guidelines
1. First adjust `bias_fo` to reduce high-frequency light fluctuations
2. Then adjust `bias_hpf` to reduce low-frequency background noise
3. Finally fine-tune `bias_diff_on` and `bias_diff_off` for contrast sensitivity
4. **Note**: `bias_diff` is the base differential bias and is recommended to not change

### BiasConfig Usage
```cpp
// BiasConfig is automatically created by ConcreteCameraStream
auto concreteCam = std::dynamic_pointer_cast<ConcreteCameraStream>(cameraStream);
if (concreteCam) {
    registry->addConfig("Bias Settings", concreteCam->getBiasConfig());
}
```

### Metavision SDK Integration
Uses `Metavision::I_LL_Biases` facility from HAL:
```cpp
auto *biases = camera->get_device().get_facility<Metavision::I_LL_Biases>();
biases->set("bias_diff", value);
int current = biases->get("bias_diff");
```

## WHEN MAKING CHANGES

### Before Starting
1. Read this entire agent.md file
2. Check existing code structure in `include/` and `src/`
3. Verify no similar functionality exists
4. Understand the interface patterns (`ICameraStream`, `IConfig`)

### During Development
1. Maintain single entry point (`src/test.cpp`)
2. Implement `ICameraStream` for new camera sources
3. Implement `IConfig` for new configurable components
4. Register configs with `Registry::instance()`
5. Use spdlog for logging (not cout/printf)
6. Ensure thread safety for shared resources

### After Changes
1. Update THIS agent.md if architecture changes
2. Do NOT create new README or CHANGELOG files
3. Clean up any test files
4. Verify CMake build still works
5. Test with both real camera and mock mode

## BUILDING THE PROJECT

```bash
# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run
./main
```

## TROUBLESHOOTING

### Common Issues
1. **No Metavision SDK**: Application falls back to MockCameraStream automatically
2. **OpenGL Errors**: Ensure OpenGL 3.2+ support on your system
3. **Threading Issues**: Check TBB installation, verify thread count
4. **ImGui Issues**: Ensure imgui submodule is initialized (`git submodule update --init`)

### Logging
- spdlog output shows initialization, camera status, and errors
- Enable verbose mode in GUI for detailed event processing logs

## CRITICAL REMINDERS

1. **NO NEW README FILES** - Use existing README.md
2. **NO CHANGELOG FILES** - Updates go in existing docs
3. **KEEP test.cpp AS ENTRY** - Don't create alternative main() functions
4. **USE INTERFACES** - New cameras implement ICameraStream, configs implement IConfig
5. **THREAD SAFETY** - Use mutex/atomic for shared state
6. **USE SPDLOG** - Not cout/printf for logging
7. **IMGUI DOCKING** - Use the docking branch features appropriately
8. **C++20** - Use modern C++ features where beneficial

## CURRENT STATE

- **Status**: Fully functional application
- **Camera Support**: Metavision SDK + Mock fallback
- **GUI**: Complete ImGui interface with menus and settings
- **Threading**: Multi-threaded event processing operational
- **Known Issues**: None currently documented
- **Ready**: For production use

---

**Last Updated**: Current version with ImGui interfaces
**Primary Entry**: src/test.cpp
**Build System**: CMake
**Documentation**: This agent.md + README.md (no more needed)
