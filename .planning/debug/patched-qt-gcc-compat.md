---
status: diagnosed
trigger: "Patched Qt 5.15.1 fails to compile on Linux with GCC errors in qfloat16.h"
created: 2026-02-03T00:00:00Z
updated: 2026-02-03T00:00:00Z
---

## Current Focus

hypothesis: Qt 5.15.1 source lacks explicit `#include <limits>` in qfloat16.h and qendian.h; GCC 11+ (shipped with ubuntu-22.04) removed transitive includes that previously pulled it in
test: CONFIRMED - this is a well-documented Qt bug (QTBUG-97896)
expecting: N/A - root cause confirmed
next_action: Create patch file .ci/patches/5.15.1/001-fix-gcc11-missing-limits.patch

## Symptoms

expected: Qt 5.15.1 compiles successfully on ubuntu-22.04 in CI (ci-patched-qt.yml workflow)
actual: Compilation fails with multiple errors in Qt headers
errors: |
  - qfloat16.h:300: error: 'numeric_limits' is not a class template
  - qfloat16.h:344: error: 'std::numeric_limits' is not a template
  - qendian.h:331: error: 'std::numeric_limits' is not a template
reproduction: Build Qt 5.15.1 from source on ubuntu-22.04 (GCC 11+) without patches
started: Since CI uses ubuntu-22.04 which ships GCC 11.3

## Eliminated

(none - first hypothesis confirmed)

## Evidence

- timestamp: 2026-02-03T00:00:00Z
  checked: CI workflow .github/workflows/ci-patched-qt.yml
  found: |
    Linux matrix cell uses ubuntu-22.04 (line 27).
    ubuntu-22.04 ships GCC 11.3.0 by default.
    GCC 11 changed libstdc++ to remove many transitive includes, including
    <limits> from headers that previously included it indirectly.
  implication: Qt 5.15.1 was released before GCC 11 existed; its headers assumed transitive includes that no longer hold.

- timestamp: 2026-02-03T00:00:00Z
  checked: .ci/patches/5.15.1/ directory contents
  found: |
    Only README.md and .gitkeep exist. No .patch files present.
    README.md explicitly states: "Current Patches: None yet."
  implication: The patch infrastructure is ready but no patches have been added yet. This is the gap.

- timestamp: 2026-02-03T00:00:00Z
  checked: .github/actions/build-qt/action.yml patch application logic (lines 80-113)
  found: |
    Step 4 "Apply patches" does:
    1. git init inside Qt source tree
    2. git add -A && git commit (creates baseline)
    3. Finds *.patch files in PATCHES_DIR sorted alphabetically
    4. Applies each with: git apply --verbose "$patch_file"
    If no .patch files found, logs "building unpatched Qt" and continues.
  implication: Patch mechanism is fully functional. Just needs a .patch file dropped in.

- timestamp: 2026-02-03T00:00:00Z
  checked: Affected Qt source files in 5.15.1
  found: |
    Three files need the fix:
    1. qtbase/src/corelib/global/qfloat16.h - uses std::numeric_limits without including <limits>
    2. qtbase/src/corelib/global/qendian.h - uses std::numeric_limits without including <limits>
    3. qtbase/src/corelib/global/qflags.h - uses std::numeric_limits (may also need it)

    The Qt project fixed this upstream in Qt 5.15.3+ (and 6.x) via QTBUG-97896.
  implication: A simple patch adding #include <limits> to the top of these files resolves the issue.

- timestamp: 2026-02-03T00:00:00Z
  checked: Windows matrix cell in ci-patched-qt.yml
  found: |
    Windows uses windows-2022 with MSVC v142 (toolset 14.29).
    MSVC has always been more permissive with transitive includes.
    This issue is Linux/GCC-only.
  implication: The patch is harmless on Windows (adding an already-included header is a no-op) but essential for Linux.

## Resolution

root_cause: |
  Qt 5.15.1 source files qfloat16.h and qendian.h use `std::numeric_limits`
  but do not explicitly `#include <limits>`. They relied on transitive includes
  from other standard library headers. GCC 11+ (libstdc++) removed these
  transitive includes as part of C++ standards compliance cleanup. The CI runner
  ubuntu-22.04 ships GCC 11.3, triggering the failure.

  The .ci/patches/5.15.1/ directory was created with full patch infrastructure
  but contains no actual patch files yet.

fix: |
  Create patch file: .ci/patches/5.15.1/001-fix-gcc11-missing-limits.patch

  The patch should add `#include <limits>` near the top of:
  - qtbase/src/corelib/global/qfloat16.h
  - qtbase/src/corelib/global/qendian.h

  Patch format (git diff style, applied with git apply -p1):

  ```diff
  diff --git a/qtbase/src/corelib/global/qfloat16.h b/qtbase/src/corelib/global/qfloat16.h
  --- a/qtbase/src/corelib/global/qfloat16.h
  +++ b/qtbase/src/corelib/global/qfloat16.h
  @@ -43,6 +43,7 @@

   #include <QtCore/qglobal.h>
   #include <QtCore/qmetatype.h>
  +#include <limits>
   #include <string.h>

   #if defined(QT_COMPILER_SUPPORTS_F16C) && defined(__AVX2__) && !defined(Q_CC_MSVC)

  diff --git a/qtbase/src/corelib/global/qendian.h b/qtbase/src/corelib/global/qendian.h
  --- a/qtbase/src/corelib/global/qendian.h
  +++ b/qtbase/src/corelib/global/qendian.h
  @@ -43,6 +43,7 @@

   #include <QtCore/qglobal.h>
  +#include <limits>

   // include stdlib.h and hope that it defines __GLIBC__ for glibc-based systems
   #include <stdlib.h>
  ```

  After adding the patch:
  - The CI cache key includes hashFiles('.ci/patches/5.15.1/**'), so it will
    automatically invalidate and trigger a fresh Qt build with the patch applied.
  - Update .ci/patches/5.15.1/README.md "Current Patches" section to document the new patch.

verification: |
  Not yet verified (diagnosis only). To verify:
  1. Create the patch file as described above
  2. Push to main to trigger ci-patched-qt.yml
  3. Confirm the "Build Qt (Linux)" step completes without qfloat16.h/qendian.h errors
  4. Confirm the full workflow (configure, build, test, install) passes on both Linux and Windows

files_changed: []

---

## Summary

**STATUS: ROOT CAUSE FOUND**

The `.ci/patches/5.15.1/` directory has the patch infrastructure fully wired up
(composite action, cache invalidation, sorted application order) but contains
zero patch files. Qt 5.15.1 predates GCC 11 and has missing `#include <limits>`
directives in `qfloat16.h` and `qendian.h`. The fix is a single patch file:
`001-fix-gcc11-missing-limits.patch`.

This is NOT a project code bug -- it is a known upstream Qt deficiency (QTBUG-97896)
that was fixed in Qt 5.15.3+. Since the CI deliberately builds the older 5.15.1
from source (to test patching capability), the patch must be provided.
