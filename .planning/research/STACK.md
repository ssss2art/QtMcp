# Technology Stack

**Project:** QtMCP
**Researched:** 2026-01-29
**Overall Confidence:** HIGH (verified against official docs and current releases)

---

## Executive Summary

QtMCP requires a carefully chosen stack that balances Qt integration, cross-platform compatibility (Windows + Linux), and ease of development. The core constraint is Qt 5.15 LTS as primary target with C++17, while the Python MCP server uses the official Anthropic SDK.

The recommended stack prioritizes:
1. **Native Qt modules** where available (WebSockets, JSON) to minimize dependencies
2. **vcpkg manifest mode** for third-party dependency management
3. **Official MCP Python SDK** for the server component
4. **Modern CMake patterns** with presets for cross-platform builds

---

## C++ Probe Stack

### Core Framework

| Technology | Version | Purpose | Confidence | Rationale |
|------------|---------|---------|------------|-----------|
| **Qt 5.15 LTS** | 5.15.x | Primary target framework | HIGH | Industry LTS standard, supported until 2025+. Most deployed Qt apps use 5.15. GammaRay supports 5.15+. |
| **Qt 6.3+** | 6.3 - 6.10 | Secondary target | HIGH | Qt 6 requires C++17. Version 6.3+ supported by GammaRay 3.1.0. Current is Qt 6.10 (Oct 2025). |
| **C++17** | ISO C++17 | Language standard | HIGH | Required by Qt 6. Supported by Qt 5.15. Provides `std::optional`, structured bindings, `if constexpr`. |

**Why C++17 (not C++20):**
- Qt 5.15 builds with C++17 out of the box
- Qt 6 requires C++17 minimum
- C++20 has inconsistent compiler support across MSVC/GCC versions
- C++20 features (modules, coroutines) not needed for this project

### Compilers

| Platform | Compiler | Minimum Version | Recommended | Confidence |
|----------|----------|-----------------|-------------|------------|
| **Windows** | MSVC 2022 | 17.0 | 17.8+ | HIGH | Qt 6.10 officially supports MSVC 2022. Full C++17 support. |
| **Windows** | MinGW-w64 | 8.1 | 13.1 | HIGH | Qt 6.10 bundles MinGW 13.1. Older versions work for Qt 5.15. |
| **Linux** | GCC | 9.0 | 11+ | HIGH | GCC 9 minimum for Qt 6. GCC 11+ recommended for better C++17 conformance. |
| **Linux** | Clang | 10.0 | 15+ | MEDIUM | Clang works but GCC is more commonly tested with Qt on Linux. |

**Why NOT Clang on Windows:**
- clang-cl has Qt build quirks
- MSVC is the path of least resistance on Windows
- Better debugging experience with Visual Studio

### Qt Modules Required

| Module | Package | Purpose | Confidence |
|--------|---------|---------|------------|
| **Qt Core** | qtbase | QObject, meta-object system, signals/slots | HIGH |
| **Qt Network** | qtbase | TCP networking foundation | HIGH |
| **Qt WebSockets** | qtwebsockets | WebSocket server for probe communication | HIGH |
| **Qt GUI** | qtbase | Screenshot capture, widget access | HIGH |
| **Qt Widgets** | qtbase | Widget introspection (if target uses Widgets) | HIGH |

### WebSocket Library

| Technology | Version | Purpose | Confidence | Rationale |
|------------|---------|---------|------------|-----------|
| **Qt WebSockets** | (bundled) | WebSocket server in probe | HIGH | Native Qt integration, no external deps, signal/slot based |

**Why Qt WebSockets (not alternatives):**
- **websocketpp**: Requires Boost or standalone ASIO - unnecessary dependency
- **Boost.Beast**: Heavyweight, requires Boost
- **libwebsockets**: C library, doesn't integrate well with Qt event loop
- Qt WebSockets uses Qt's event loop natively, making it trivial to integrate with the rest of the probe

**CMake:**
```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets)
# or for Qt 5:
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Network WebSockets)
```

### JSON Library

| Technology | Version | Purpose | Confidence | Rationale |
|------------|---------|---------|------------|-----------|
| **QJsonDocument** | (bundled) | JSON-RPC parsing/generation | HIGH | Part of Qt Core, no external deps, sufficient for protocol |
| **nlohmann/json** | 3.12.0 | *Optional alternative* | HIGH | Better ergonomics, but adds dependency |

**Recommendation: Use QJsonDocument**

**Why QJsonDocument (not nlohmann/json):**
- Zero external dependencies
- Native QString/QVariant integration
- Sufficient for JSON-RPC 2.0 protocol needs
- Already part of Qt Core (no vcpkg needed)

**When to consider nlohmann/json:**
- Only if complex JSON manipulation becomes unwieldy
- If team strongly prefers modern C++ API (`json["key"]` vs `obj.value("key")`)

**Performance is not a concern:** JSON-RPC messages are small. The 3-4x performance difference between nlohmann and RapidJSON is irrelevant here.

### Testing Framework

| Technology | Version | Purpose | Confidence | Rationale |
|------------|---------|---------|------------|-----------|
| **Qt Test** | (bundled) | Unit tests for probe | HIGH | Native Qt signal/slot testing, QSignalSpy, zero config |
| **Catch2** | 3.12.0 | *Alternative* | MEDIUM | Better assertion macros, but requires vcpkg |

**Recommendation: Qt Test for probe, Catch2 optional for pure C++ utilities**

**Why Qt Test:**
- `QSignalSpy` for testing signals - essential for this project
- `QCOMPARE`, `QVERIFY` work well with Qt types
- `QTest::mouseClick`, `QTest::keyClick` for UI testing
- Qt Creator has built-in support
- No additional dependency

**Why NOT GoogleTest:**
- Heavier setup than Catch2
- No Qt-specific features
- Catch2 is preferred if you need non-Qt testing

---

## Build System

### CMake Configuration

| Technology | Version | Purpose | Confidence |
|------------|---------|---------|------------|
| **CMake** | 3.21+ | Build system | HIGH |
| **CMakePresets.json** | Version 3+ | Build configuration | HIGH |
| **vcpkg** | Latest | Dependency management | HIGH |

**Why CMake 3.21+:**
- `CMakePresets.json` support (added 3.19, matured 3.21)
- Better Qt6 integration
- `CMAKE_TOOLCHAIN_FILE` works reliably with presets

### vcpkg Manifest Mode

**Recommended:** Use vcpkg manifest mode with `vcpkg.json` in project root.

**vcpkg.json example:**
```json
{
  "name": "qtmcp",
  "version": "0.1.0",
  "dependencies": [
    "catch2"
  ],
  "overrides": [],
  "builtin-baseline": "2024.01.12"
}
```

**Why NOT include Qt in vcpkg:**
- Qt via vcpkg is complex and slow to build
- Most users have Qt installed via official installer
- QtMCP must build against user's Qt installation (matches GammaRay approach)

**Why vcpkg for other deps:**
- Cross-platform package management
- CMake integration via toolchain file
- Manifest mode tracks deps in git

### CMakePresets.json Structure

```json
{
  "version": 6,
  "cmakeMinimumRequired": { "major": 3, "minor": 21, "patch": 0 },
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_CXX_STANDARD": "17",
        "CMAKE_CXX_STANDARD_REQUIRED": "ON",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "windows-msvc",
      "inherits": "base",
      "condition": { "type": "equals", "lhs": "${hostSystemName}", "rhs": "Windows" },
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "VCPKG_TARGET_TRIPLET": "x64-windows"
      }
    },
    {
      "name": "linux-gcc",
      "inherits": "base",
      "condition": { "type": "equals", "lhs": "${hostSystemName}", "rhs": "Linux" },
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "VCPKG_TARGET_TRIPLET": "x64-linux"
      }
    }
  ],
  "buildPresets": [
    { "name": "windows-debug", "configurePreset": "windows-msvc", "configuration": "Debug" },
    { "name": "windows-release", "configurePreset": "windows-msvc", "configuration": "Release" },
    { "name": "linux-debug", "configurePreset": "linux-gcc", "configuration": "Debug" },
    { "name": "linux-release", "configurePreset": "linux-gcc", "configuration": "Release" }
  ]
}
```

---

## Injection Mechanism

### Windows: DLL Injection

| Technique | Confidence | Rationale |
|-----------|------------|-----------|
| **CreateRemoteThread + LoadLibrary** | HIGH | Standard technique, used by GammaRay's `windll` injector |
| Launch with injector EXE | HIGH | Simpler than attaching - matches MVP scope |

**Implementation approach:**
1. `qtmcp-launch.exe` spawns target process suspended
2. Allocates memory in target, writes DLL path
3. `CreateRemoteThread` calls `LoadLibraryW`
4. Resume target process

**Library option:** [kubo/injector](https://github.com/kubo/injector) - cross-platform injection library with Windows and Linux support.

### Linux: LD_PRELOAD

| Technique | Confidence | Rationale |
|-----------|------------|-----------|
| **LD_PRELOAD** | HIGH | Standard technique, used by GammaRay, Qt-Inspector, Qat |
| Launch wrapper script | HIGH | Simplest approach for MVP |

**Implementation approach:**
```bash
LD_PRELOAD=/path/to/libqtmcp-probe.so ./target-app
```

**Probe initialization:**
- Use `__attribute__((constructor))` function
- Or hook into Qt via `qtHookData` (GammaRay approach)

---

## Python MCP Server Stack

### Core Framework

| Technology | Version | Purpose | Confidence | Rationale |
|------------|---------|---------|------------|-----------|
| **Python** | 3.10+ | Runtime | HIGH | MCP SDK requires 3.10+. 3.10 is lowest common denominator. |
| **mcp (official SDK)** | 1.26.0 | MCP protocol implementation | HIGH | Official Anthropic SDK, production-ready, stable v1.x |

**Why official MCP SDK (not FastMCP 2.0/3.0):**
- Official SDK is maintained by Anthropic
- FastMCP 1.0 was incorporated into official SDK
- FastMCP 2.0/3.0 adds complexity not needed for this project
- Official SDK provides `mcp.server.fastmcp.FastMCP` for simple server creation

### WebSocket Client

| Technology | Version | Purpose | Confidence | Rationale |
|------------|---------|---------|------------|-----------|
| **websockets** | 16.0 | WebSocket client for probe communication | HIGH | asyncio-native, production stable, simple API |

**Why websockets (not aiohttp):**
- Dedicated WebSocket library (aiohttp is full HTTP framework)
- Simpler API: `await ws.recv()` / `await ws.send()`
- Better performance for WebSocket-only use case
- Recommended for async applications in 2025

**Why NOT websocket-client:**
- Synchronous/threaded - doesn't fit MCP SDK's async model
- MCP SDK uses asyncio natively

### Additional Python Dependencies

| Technology | Version | Purpose | Confidence |
|------------|---------|---------|------------|
| **pydantic** | 2.x | Data validation (used by MCP SDK) | HIGH |
| **pytest** | 8.x | Testing | HIGH |
| **pytest-asyncio** | 0.23+ | Async test support | HIGH |

### Python Package Structure

```
qtmcp/
  pyproject.toml
  src/
    qtmcp/
      __init__.py
      server.py      # MCP server
      client.py      # WebSocket client to probe
      protocol.py    # JSON-RPC message types
  tests/
    conftest.py
    test_server.py
    test_client.py
```

**pyproject.toml dependencies:**
```toml
[project]
name = "qtmcp"
requires-python = ">=3.10"
dependencies = [
    "mcp>=1.26.0",
    "websockets>=16.0",
]

[project.optional-dependencies]
dev = [
    "pytest>=8.0",
    "pytest-asyncio>=0.23",
]
```

---

## What NOT to Use

| Technology | Why Avoid |
|------------|-----------|
| **Boost** | Heavyweight, unnecessary when Qt provides equivalent functionality |
| **RapidJSON** | Performance overkill, QJsonDocument sufficient |
| **nlohmann/json** | Optional - adds dependency when QJsonDocument works |
| **websocketpp** | Requires Boost/ASIO, Qt WebSockets is better integrated |
| **GoogleTest** | Qt Test is better for Qt-specific testing; Catch2 if you need non-Qt |
| **FastMCP 2.0/3.0** | Unnecessary complexity over official SDK for this project |
| **aiohttp** | Overkill for WebSocket-only client; websockets is simpler |
| **Qt from vcpkg** | Complex, slow builds; use official Qt installer instead |
| **Clang on Windows** | MSVC is better tested and debugged with Qt |
| **C++20** | Inconsistent compiler support, C++17 is the sweet spot |

---

## Version Summary Table

| Component | Recommended Version | Min Version | Notes |
|-----------|---------------------|-------------|-------|
| C++ Standard | C++17 | C++17 | Required by Qt 6, supported by Qt 5.15 |
| Qt | 5.15 LTS / 6.3+ | 5.15 | 5.15 primary, 6.x secondary |
| CMake | 3.25 | 3.21 | For CMakePresets.json support |
| MSVC | 2022 (17.8+) | 2019 | Qt 6.10 bundles 2022 |
| GCC | 11+ | 9 | Qt 6 minimum is GCC 9 |
| vcpkg | Latest | 2024.01 | Manifest mode |
| Python | 3.11 | 3.10 | MCP SDK requirement |
| MCP SDK | 1.26.0 | 1.20 | Official Anthropic SDK |
| websockets | 16.0 | 14.0 | Async WebSocket client |
| Catch2 | 3.12.0 | 3.0 | Optional, for non-Qt tests |

---

## Installation Commands

### Windows (MSVC)

```powershell
# Install vcpkg (if not already)
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat

# Set environment variable
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# Configure and build
cmake --preset windows-msvc
cmake --build --preset windows-debug
```

### Linux (GCC)

```bash
# Install vcpkg (if not already)
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# Set environment variable
export VCPKG_ROOT=/path/to/vcpkg

# Configure and build
cmake --preset linux-gcc
cmake --build --preset linux-debug
```

### Python Environment

```bash
# Create virtual environment
python -m venv .venv
source .venv/bin/activate  # Linux
.venv\Scripts\activate     # Windows

# Install dependencies
pip install -e ".[dev]"
```

---

## Sources

### Verified (HIGH Confidence)
- [Qt 6.10 Supported Platforms](https://doc.qt.io/qt-6/supported-platforms.html) - Compiler requirements
- [Qt WebSockets Documentation](https://doc.qt.io/qt-6/qtwebsockets-index.html) - Native WebSocket module
- [MCP Python SDK (PyPI)](https://pypi.org/project/mcp/) - Version 1.26.0, Python 3.10+
- [websockets (PyPI)](https://pypi.org/project/websockets/) - Version 16.0, Python 3.10+
- [nlohmann/json Releases](https://github.com/nlohmann/json/releases) - Version 3.12.0 (April 2025)
- [Catch2 GitHub](https://github.com/catchorg/Catch2) - Version 3.12.0, C++14 minimum
- [GammaRay GitHub](https://github.com/KDAB/GammaRay) - Architecture reference, Qt 5.15+/6.3+ support
- [vcpkg CMake Integration](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration) - Manifest mode docs

### Cross-Referenced (MEDIUM Confidence)
- [Qt Wiki: Supported C++ Versions](https://wiki.qt.io/Supported_C++_Versions) - C++ standard requirements
- [kubo/injector](https://github.com/kubo/injector) - Cross-platform injection library
- [Qat Architecture](https://qat.readthedocs.io/en/1.2.0/doc/contributing/Architecture.html) - Similar project reference
