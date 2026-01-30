---
phase: 02-core-introspection
plan: 05
subsystem: introspection
tags: [signals, subscriptions, lifecycle, notifications, qmetamethod]

# Dependency graph
requires:
  - phase: 02-01
    provides: ObjectRegistry with objectAdded/objectRemoved signals
  - phase: 02-03
    provides: MetaInspector for signal discovery
provides:
  - SignalMonitor singleton for signal subscriptions
  - Dynamic signal-to-slot connections via SignalRelay pattern
  - Object lifecycle notifications (created/destroyed)
  - Automatic unsubscription on object destruction
affects: [02-06-json-rpc, phase-3-protocol]

# Tech tracking
tech-stack:
  added: []
  patterns: [SignalRelay pattern for dynamic signal connections, DirectConnection for cleanup with QueuedConnection for notification]

key-files:
  created:
    - src/probe/introspection/signal_monitor.h
    - src/probe/introspection/signal_monitor.cpp
    - tests/test_signal_monitor.cpp
  modified:
    - src/probe/CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "SignalRelay helper class for dynamic signal connections via QMetaMethod"
  - "DirectConnection for destruction cleanup, caching objectId for QueuedConnection notification"
  - "m_destroyedObjectIds cache bridges timing gap between DirectConnection and QueuedConnection"

patterns-established:
  - "SignalRelay: Per-subscription QObject with handleSignal() slot for QMetaMethod connections"
  - "Lifecycle notification timing: DirectConnection caches ID, QueuedConnection emits notification"

# Metrics
duration: 18min
completed: 2026-01-30
---

# Phase 02 Plan 05: Signal Monitor Summary

**SignalMonitor singleton enabling signal subscriptions with dynamic QMetaMethod connections via SignalRelay pattern and lifecycle notifications**

## Performance

- **Duration:** 18 min
- **Started:** 2026-01-30T14:40:00Z
- **Completed:** 2026-01-30T14:58:00Z
- **Tasks:** 3 (Task 1 and 2 already committed together by external process)
- **Files modified:** 5

## Accomplishments
- SignalMonitor singleton with Q_GLOBAL_STATIC for signal subscription management
- Dynamic signal connections using SignalRelay helper and QMetaObject::connect
- Lifecycle notifications (objectCreated/objectDestroyed) connected to ObjectRegistry
- Auto-unsubscribe when subscribed objects are destroyed
- 8 comprehensive unit tests covering all subscription and lifecycle scenarios

## Task Commits

Tasks 1 and 2 (SignalMonitor class creation and ObjectRegistry lifecycle connection) were already committed in an earlier session as part of commit `4b1358a`.

3. **Task 3: Add unit tests for SignalMonitor** - `135e905` (test)

## Files Created/Modified
- `src/probe/introspection/signal_monitor.h` - SignalMonitor class declaration with Subscription struct
- `src/probe/introspection/signal_monitor.cpp` - Implementation with SignalRelay helper class
- `src/probe/CMakeLists.txt` - Added signal_monitor.cpp/h to build
- `tests/test_signal_monitor.cpp` - 8 test cases for signal monitoring
- `tests/CMakeLists.txt` - Added test_signal_monitor target

## Decisions Made

1. **SignalRelay helper class for dynamic signal connections**
   - Qt6 QMetaObject::connect requires QMetaMethod for both signal and slot
   - SignalRelay provides a generic handleSignal() slot that can receive any signal
   - Each subscription creates one SignalRelay instance capturing subscription context
   - Relay emits signalTriggered(QJsonObject) which forwards to signalEmitted

2. **m_destroyedObjectIds cache for lifecycle destroyed notifications**
   - ObjectRegistry::objectRemoved is delivered via QueuedConnection
   - QObject::destroyed connected via DirectConnection fires synchronously
   - By the time onObjectRemoved runs, object and its ID are already gone from registry
   - Solution: onSubscribedObjectDestroyed caches objectId in m_destroyedObjectIds
   - onObjectRemoved reads and removes cached ID for notification

3. **Lifecycle notifications are opt-in (disabled by default)**
   - Can be very noisy in large applications
   - setLifecycleNotificationsEnabled(true) to enable

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed crash in onObjectRemoved when accessing destroyed object**
- **Found during:** Task 3 (running testLifecycleDestroyed)
- **Issue:** objectId(obj) called on destroyed object caused segfault in generateObjectId
- **Fix:** Added m_destroyedObjectIds cache in header, populated in DirectConnection callback, read in QueuedConnection handler
- **Files modified:** signal_monitor.h, signal_monitor.cpp
- **Verification:** testLifecycleDestroyed passes without crash
- **Committed in:** 135e905 (Task 3 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Bug fix essential for correct lifecycle notification delivery. No scope creep.

## Issues Encountered
- Initial connect() approach using string-based signals with lambda failed (Qt6 doesn't support this pattern)
- Solution: Created SignalRelay helper class with proper QMetaMethod slot

## Next Phase Readiness
- SignalMonitor ready for JSON-RPC integration in 02-06
- subscribe/unsubscribe methods available for signal_subscribe/signal_unsubscribe RPC calls
- signalEmitted signal can be forwarded as JSON-RPC notifications

---
*Phase: 02-core-introspection*
*Completed: 2026-01-30*
