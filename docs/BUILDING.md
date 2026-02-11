# Building QtMcp from Source

This guide covers building the QtMcp probe and launcher from source code.

## Prerequisites

### Required

- **CMake 3.16+**
- **Qt 5.15.1+ or Qt 6.5+** with development headers (including private headers)
- **C++17 compiler:**
  - GCC 8+ (Linux)
  - Clang 7+ (Linux/macOS)
  - MSVC 2019+ (Windows)

### Required Qt Modules

- Qt Core (including CorePrivate for private headers)
- Qt Network
- Qt WebSockets
- Qt Widgets
- Qt Test (for building tests)

### Optional

- **Qt Qml/Quick** - Enables QML introspection

## Clone and Build

### Basic Build

```bash
git clone https://github.com/ssss2art/QtMcp.git
cd QtMcp

# Configure (specify Qt path if not in system PATH)
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64

# Build
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure
```

### Windows with Visual Studio

```powershell
git clone https://github.com/ssss2art/QtMcp.git
cd QtMcp

# Configure for Visual Studio 2022
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"

# Build
cmake --build build --config Release

# Run tests
ctest --test-dir build --output-on-failure -C Release
```

## Using CMake Presets

The project includes predefined presets for common configurations.

### Available Presets

| Preset | Platform | Build Type | Description |
|--------|----------|------------|-------------|
| `debug` | Linux | Debug | Development build with tests |
| `release` | Linux | Release | Optimized Linux build |
| `windows-debug` | Windows | Debug | Development build with tests |
| `windows-release` | Windows | Release | Optimized Windows build |

Presets are platform-conditional -- only the presets for your current OS will appear.

### Using Presets

```bash
# List available presets
cmake --list-presets

# Configure with a preset
cmake --preset release

# Build with the preset
cmake --build --preset release

# Run tests with the preset
ctest --preset release
```

### Custom Qt Path with Presets

Pass `-DCMAKE_PREFIX_PATH` after the preset to point at a custom Qt installation:

```bash
cmake --preset release -DCMAKE_PREFIX_PATH=/opt/Qt/6.7.2/gcc_64
```

For a persistent local override, create a `CMakeUserPresets.json` (not checked in) that inherits from an existing preset:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "my-qt",
      "inherits": "windows-release",
      "cacheVariables": {
        "CMAKE_PREFIX_PATH": "C:/Qt/6.7.2/msvc2022_64"
      }
    }
  ]
}
```

Then use `cmake --preset my-qt`.

## Build Options

Configure these options with `-D<OPTION>=<VALUE>`:

| Option | Default | Description |
|--------|---------|-------------|
| `QTMCP_BUILD_TESTS` | `ON` | Build unit tests |
| `QTMCP_BUILD_TEST_APP` | `ON` | Build the test Qt application |
| `QTMCP_QT_DIR` | - | Explicit path to Qt installation (prepended to `CMAKE_PREFIX_PATH`) |

### Examples

```bash
# Build without tests (faster)
cmake -B build -DQTMCP_BUILD_TESTS=OFF -DQTMCP_BUILD_TEST_APP=OFF

# Specify Qt installation directly
cmake -B build -DQTMCP_QT_DIR=/opt/Qt/6.8.0/gcc_64
```

## Build Artifacts

After building, find the artifacts in these locations:

### Probe Library

| Platform | Location |
|----------|----------|
| Windows | `build/lib/Release/qtmcp-probe-qt6.8.dll` |
| Linux | `build/lib/libqtmcp-probe-qt6.8.so` |

The probe binary name includes the Qt major.minor version it was built against (e.g. `qt6.8`, `qt5.15`).

### Launcher Executable

| Platform | Location |
|----------|----------|
| Windows | `build/bin/Release/qtmcp-launcher.exe` |
| Linux | `build/bin/qtmcp-launcher` |

### Test App (if enabled)

| Platform | Location |
|----------|----------|
| Windows | `build/bin/Release/qtmcp-test-app.exe` |
| Linux | `build/bin/qtmcp-test-app` |

## Running Tests

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
ctest --test-dir build -R "test_object_registry" --output-on-failure

# Verbose output
ctest --test-dir build -V
```

On Windows with multi-config generators:
```powershell
ctest --test-dir build --output-on-failure -C Release
```

## Installation

```bash
# Install to default prefix (/usr/local on Linux)
cmake --install build

# Install to custom prefix
cmake --install build --prefix /path/to/install

# Install specific configuration (Windows)
cmake --install build --config Release --prefix C:\QtMcp
```

Installation layout:
```
<prefix>/
├── bin/
│   └── qtmcp-launcher(.exe)
├── lib/
│   ├── qtmcp-probe-qt6.8.dll  (Windows)
│   └── libqtmcp-probe-qt6.8.so (Linux)
├── include/qtmcp/
│   └── (header files)
└── share/cmake/QtMCP/
    └── (CMake package files)
```

## Qt Version Notes

### Qt 5.15

Qt 5.15 requires private headers for probe functionality. Ensure your Qt installation includes them:

```bash
# Check for private headers
ls /path/to/Qt/5.15.2/gcc_64/include/QtCore/5.15.2/QtCore/private/
# Should contain qhooks_p.h
```

### Qt 6.x

Qt 6.5+ is fully supported. Qt 6.x changed some internal APIs, but QtMcp handles this transparently via compatibility helpers in `src/compat/`.

### Building for Multiple Qt Versions

To create probes for different Qt versions, configure and build separately:

```bash
# Build for Qt 6.8
cmake -B build-qt6 -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64
cmake --build build-qt6

# Build for Qt 5.15
cmake -B build-qt5 -DCMAKE_PREFIX_PATH=/path/to/Qt/5.15.2/gcc_64
cmake --build build-qt5
```

## Troubleshooting Build Issues

### "Qt6::CorePrivate not found"

Your Qt installation is missing private development headers. Solutions:

1. **apt (Ubuntu/Debian):** `sudo apt install qt6-base-private-dev`
2. **aqt (online installer):** Ensure you installed with `--modules all`
3. **Manual:** Check that `<qt-install>/include/QtCore/<version>/QtCore/private/` exists

### "Could not find Qt"

Set the Qt path explicitly:

```bash
cmake -B build -DQTMCP_QT_DIR=/path/to/Qt/6.8.0/gcc_64
# or
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64
```

### Compiler Version Errors

QtMcp requires C++17. Check your compiler version:

```bash
g++ --version   # Need 8+
clang++ --version  # Need 7+
```

On Windows, Visual Studio 2019 or later is required.

## IDE Setup

### Visual Studio Code

1. Install CMake Tools extension
2. Open the QtMcp folder
3. Select a configure preset or set `CMAKE_PREFIX_PATH` in settings.json
4. Build and debug from the CMake panel

### CLion

1. Open the CMakeLists.txt as a project
2. Set `CMAKE_PREFIX_PATH` in CMake options: `-DCMAKE_PREFIX_PATH=/path/to/Qt`
3. Select build configuration and build

### Visual Studio

1. Open folder or CMakeLists.txt
2. Configure Qt path in CMakeSettings.json
3. Build from the Build menu

## Next Steps

- [Getting Started](GETTING-STARTED.md) - Use the probe with your application
- [Troubleshooting](TROUBLESHOOTING.md) - Common runtime issues
