---
status: resolved
trigger: "CI build failure: probe.cpp:223 has 'invalid use of incomplete type class QJsonDocument' error"
created: 2026-02-03T00:00:00Z
updated: 2026-02-03T00:00:00Z
---

## Current Focus

hypothesis: Missing #include <QJsonDocument> in probe.cpp
test: Check includes in probe.cpp vs usage of QJsonDocument
expecting: No QJsonDocument include present
next_action: Add the missing include

## Symptoms

expected: CI build succeeds; probe.cpp compiles without errors
actual: Build fails with "invalid use of incomplete type 'class QJsonDocument'" at lines 223, 235, 247
errors: "invalid use of incomplete type 'class QJsonDocument'" on lines 223, 235, 247 of src/probe/core/probe.cpp
reproduction: Build the project (cmake --build)
started: Unknown; likely introduced when signal notification lambdas were added to Probe::initialize()

## Eliminated

(none - root cause identified on first hypothesis)

## Evidence

- timestamp: 2026-02-03T00:00:00Z
  checked: Includes in src/probe/core/probe.cpp (lines 1-37)
  found: No #include <QJsonDocument> and no #include <QJsonObject> present in the file
  implication: QJsonDocument is an incomplete type at point of use

- timestamp: 2026-02-03T00:00:00Z
  checked: Lines 222-249 of probe.cpp
  found: Three lambda functions use QJsonDocument(rpcNotification).toJson(QJsonDocument::Compact) at lines 223, 235, 247
  implication: These lines require the full QJsonDocument definition, not just a forward declaration

- timestamp: 2026-02-03T00:00:00Z
  checked: How QJsonObject resolves (transitive includes)
  found: signal_monitor.h includes <QJsonObject>, and probe.cpp includes signal_monitor.h at line 30
  implication: QJsonObject works via transitive include, but QJsonDocument is NOT transitively available from any included header

- timestamp: 2026-02-03T00:00:00Z
  checked: All other files in src/ that use QJsonDocument
  found: 6 other .cpp files use QJsonDocument and ALL include <QJsonDocument> directly (jsonrpc_handler.cpp, symbolic_name_map.cpp, response_envelope.cpp, native_mode_api.cpp, computer_use_mode_api.cpp, chrome_mode_api.cpp)
  implication: probe.cpp is the ONLY file missing the include; all others follow the correct pattern

- timestamp: 2026-02-03T00:00:00Z
  checked: probe.h for any QJson includes
  found: probe.h includes only <QGlobalStatic> and <QObject> - no QJson headers
  implication: The header provides no transitive QJson includes either

## Resolution

root_cause: src/probe/core/probe.cpp is missing `#include <QJsonDocument>`. The file uses QJsonDocument at lines 223, 235, and 247 (inside signal notification lambdas added to Probe::initialize()) but never includes the header. QJsonObject works because it is transitively included via signal_monitor.h -> <QJsonObject>, but no transitive path provides QJsonDocument.

fix: Add `#include <QJsonDocument>` and `#include <QJsonObject>` to probe.cpp's Qt include block (after line 36, alongside other Qt includes). Both are added because relying on transitive includes is fragile - each .cpp file should include what it directly uses.

verification: Pending build verification

files_changed:
- src/probe/core/probe.cpp (added #include <QJsonDocument> and #include <QJsonObject>)
