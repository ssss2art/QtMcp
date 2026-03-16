# Bundled Test App Design

## Problem

After `pip install qtpilot && qtpilot download-tools --qt-version 6.8`, users have the probe and launcher but nothing to try them on. They need their own Qt app to get started, which kills the "just try it" experience.

## Solution

Bundle the test app with its Qt runtime DLLs in the same release archive as the probe and launcher. Add a `--demo` flag to `qtpilot serve` that auto-launches the bundled test app.

**Target quickstart:**
```bash
pip install qtpilot
qtpilot download-tools --qt-version 6.8
qtpilot serve --demo
```

## Design Decisions

- **Bundle Qt DLLs** (not static link) — uses `windeployqt` on Windows and manual lib copy on Linux. This is how most Qt apps ship and avoids static Qt licensing/build complexity.
- **Same archive** — test app goes in the existing `qtpilot-qt6.8-windows-x64.zip` alongside probe and launcher. One download gets everything.
- **All Qt versions** — every matrix entry gets the test app bundle. Users download ~20-30MB instead of ~2MB, but they only download one archive.
- **Explicit `--demo` flag** — `qtpilot serve --demo` launches the bundled test app. No implicit behavior when `--target` is omitted.
- **`testapp/` subdirectory** — on both Windows and Linux, the test app and its Qt DLLs live in a `testapp/` subdirectory within the archive. This prevents Qt DLL version conflicts with the launcher and keeps the archive structure clean.
- **macOS** — intentionally excluded; macOS support is not yet implemented in the project.

## Changes

### 1. Root CMakeLists.txt (`CMakeLists.txt`)

Add install rule for the test app (currently missing — only probe and launcher have install rules):

```cmake
# Inside the QTPILOT_BUILD_TEST_APP block (~line 283):
if(QTPILOT_BUILD_TEST_APP)
  add_subdirectory(test_app)
  install(TARGETS qtPilot_test_app RUNTIME DESTINATION bin)
endif()
```

### 2. CI Workflow (`.github/workflows/ci.yml`)

Add a post-build step that bundles the test app with its Qt dependencies. Use `${{ matrix.preset }}` (not `$PRESET`) to match existing workflow syntax. Derive `QT_DIR` from the environment variables set by `jurplel/install-qt-action` (`QT_ROOT_DIR`).

**Windows** (after `cmake --install`):
```yaml
- name: Bundle test app (Windows)
  if: runner.os == 'Windows'
  shell: cmd
  run: |
    mkdir install\${{ matrix.preset }}\testapp
    copy build\${{ matrix.preset }}\bin\Release\qtPilot-test-app.exe install\${{ matrix.preset }}\testapp\
    "%QT_ROOT_DIR%\bin\windeployqt.exe" --no-translations install\${{ matrix.preset }}\testapp\qtPilot-test-app.exe
```

Note: `--no-opengl-sw` and `--no-system-d3d-compiler` are Qt 6 flags. For Qt 5 compatibility, omit them and use only `--no-translations` which works on both.

**Linux** (after `cmake --install`):
```yaml
- name: Bundle test app (Linux)
  if: runner.os == 'Linux'
  shell: bash
  run: |
    PRESET="${{ matrix.preset }}"
    QT_MAJOR=$(echo "${{ matrix.qt }}" | cut -c1)
    mkdir -p install/$PRESET/testapp/lib install/$PRESET/testapp/plugins/platforms
    cp build/$PRESET/bin/qtPilot-test-app install/$PRESET/testapp/
    # Copy minimal Qt runtime
    for lib in libQt${QT_MAJOR}Core.so* libQt${QT_MAJOR}Gui.so* libQt${QT_MAJOR}Widgets.so* libQt${QT_MAJOR}DBus.so* libQt${QT_MAJOR}XcbQpa.so* libicui18n.so* libicuuc.so* libicudata.so*; do
      find "$QT_ROOT_DIR/lib" -name "$lib" -exec cp {} install/$PRESET/testapp/lib/ \; 2>/dev/null || true
    done
    cp "$QT_ROOT_DIR/plugins/platforms/libqxcb.so" install/$PRESET/testapp/plugins/platforms/ 2>/dev/null || true
    cp "$QT_ROOT_DIR/plugins/platforms/libqminimal.so" install/$PRESET/testapp/plugins/platforms/ 2>/dev/null || true
    # Create wrapper script
    cat > install/$PRESET/testapp/qtPilot-test-app.sh << 'WRAPPER'
#!/bin/sh
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="$DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$DIR/plugins"
exec "$DIR/qtPilot-test-app" "$@"
WRAPPER
    chmod +x install/$PRESET/testapp/qtPilot-test-app.sh
```

Also add `test_app/**` to the `on.push.paths` trigger filter so test app source changes trigger CI.

### 3. Release Workflow (`.github/workflows/release.yml`)

Update the staging step to copy the `testapp/` directory into each release archive **as a subdirectory** (not flattened):

```bash
# After copying probe + launcher into staging:
if [ -d "$dir/testapp/" ]; then
  mkdir -p "$staging/testapp"
  cp -r "$dir/testapp/"* "$staging/testapp/"
fi
```

This keeps the test app + its Qt DLLs isolated from the probe and launcher, preventing version conflicts when the user's target app uses a different Qt version.

### 4. Python Download Module (`python/src/qtpilot/download.py`)

Add a helper to locate the test app (without changing `download_and_extract()`'s return type, preserving backward compatibility):

```python
def get_testapp_path(output_dir: Path | None = None, platform_name: str | None = None) -> Path | None:
    """Find the bundled test app in the download directory."""
    if output_dir is None:
        output_dir = Path.cwd()
    if platform_name is None:
        platform_name = detect_platform()

    if platform_name == "windows":
        path = output_dir / "testapp" / "qtPilot-test-app.exe"
    else:
        path = output_dir / "testapp" / "qtPilot-test-app.sh"

    return path if path.exists() else None
```

The `download_and_extract()` return type stays `tuple[Path, Path]` — no breaking change.

### 5. CLI (`python/src/qtpilot/cli.py`)

Add `--demo` flag to the `serve` subcommand:
```python
serve_parser.add_argument(
    "--demo",
    action="store_true",
    help="Launch the bundled test app (requires download-tools first)",
)
```

Update `cmd_download_tools` to print test app path after extraction:
```python
testapp = get_testapp_path(output_dir, platform_name)
if testapp:
    print(f"Test app: {testapp}")
```

### 6. Server (`python/src/qtpilot/server.py`)

When `--demo` is set:
1. Call `get_testapp_path()` to find the test app
2. If `--target` is also set, error: "Cannot use --demo with --target"
3. If test app not found, error: "Test app not found. Run: qtpilot download-tools --qt-version <version>"
4. On Windows: set `--target` to the test app exe path. The launcher handles Qt DLL resolution from the testapp/ directory.
5. On Linux: set `--target` to the wrapper script (`qtPilot-test-app.sh`) which sets `LD_LIBRARY_PATH` and `QT_PLUGIN_PATH` before exec'ing the binary.

### 7. README

Update quickstart to show the demo path:
```markdown
### Try it out

pip install qtpilot
qtpilot download-tools --qt-version 6.8
qtpilot serve --demo

This downloads the probe, launcher, and a bundled test app.
Claude can now interact with the test app — try asking
"Show me the widget tree" or "Fill out the form".
```

## File Inventory

| File | Change |
|------|--------|
| `CMakeLists.txt` | Add `install(TARGETS qtPilot_test_app ...)` rule |
| `.github/workflows/ci.yml` | Add test app bundling step + `test_app/**` to paths |
| `.github/workflows/release.yml` | Copy `testapp/` subdirectory into staging |
| `python/src/qtpilot/download.py` | Add `get_testapp_path()` (no breaking changes) |
| `python/src/qtpilot/cli.py` | Add `--demo` flag, print test app path on download |
| `python/src/qtpilot/server.py` | Handle `--demo` flag with platform-specific logic |
| `README.md` | Update quickstart with `--demo` |

## Verification

1. Build locally — verify `windeployqt` produces a working self-contained `testapp/` directory
2. Run the bundled test app standalone (no Qt on PATH) to confirm it launches
3. `qtpilot serve --demo` launches the test app and connects
4. `qtpilot serve --demo` without download shows helpful error message
5. `qtpilot serve --demo --target foo` shows conflict error
6. Full E2E test (`/test-mcp-modes`) passes against the demo app
7. CI produces artifacts with the test app bundle
8. Release archives contain `testapp/` subdirectory with test app + Qt DLLs
9. Verify archive size is reasonable (~20-30MB, not 100MB+)
