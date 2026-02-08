---
status: investigating
trigger: "Investigate why TestMetaInspector::testVariantToJsonString() fails on Qt 5.15 Linux"
created: 2026-02-03T00:00:00Z
updated: 2026-02-03T00:10:00Z
---

## Current Focus

hypothesis: CONFIRMED - Qt5 QVariant::isNull() forwards to QString::isNull(), Qt6 removed this behavior
test: Complete - web search confirmed Qt6 breaking change
expecting: Fix should remove isNull() check to align with Qt6 behavior
next_action: Document complete findings for user

## Symptoms

expected: Test testVariantToJsonString() passes on Qt5.15 Linux
actual: Test fails with "Compared values are not the same"
errors: QCOMPARE failure on second assertion: `variantToJson(QVariant(QString())) vs QJsonValue(QString())`
reproduction: Run test on Qt 5.15 Linux
started: Unknown - Qt5 compatibility issue

## Eliminated

(none - hypothesis was correct on first attempt)

## Evidence

- timestamp: 2026-02-03T00:00:00Z
  checked: tests/test_meta_inspector.cpp lines 147-151
  found: Test has two assertions - one with "hello" string, one with empty QString()
  implication: First assertion likely passes, second fails with empty string

- timestamp: 2026-02-03T00:00:00Z
  checked: src/probe/introspection/variant_json.cpp lines 23-59
  found: variantToJson() checks !value.isValid() || value.isNull() before type switching
  implication: Empty QString() QVariant may trigger null check differently in Qt5

- timestamp: 2026-02-03T00:00:00Z
  checked: variant_json.cpp case QMetaType::QString at line 48
  found: Returns value.toString() directly without null checking
  implication: If QVariant(QString()) is valid but null, it should still return QString()

- timestamp: 2026-02-03T00:00:00Z
  checked: Qt5/Qt6 compat headers in src/compat/
  found: Compat layer handles QMetaType differences, but not QVariant null/validity differences
  implication: QVariant behavior with empty/null strings may differ between Qt versions

- timestamp: 2026-02-03T00:05:00Z
  checked: Qt6 documentation and mailing list archives
  found: Qt6 intentionally removed QVariant::isNull() forwarding to contained types
  implication: This is a deliberate breaking change, not a bug - old behavior was considered bugprone

- timestamp: 2026-02-03T00:05:00Z
  checked: Web search on "Qt5 Qt6 QVariant isNull behavior"
  found: Qt docs confirm isNull() no longer forwards in Qt6. "Qt developers introduced these behavior changes in existing methods of QVariant and were aware that silent behavior changes are a common source of bugs, but deemed the current behavior to be bugprone enough to warrant it."
  implication: The fix should align with Qt6 behavior (remove isNull() check) rather than work around Qt5 behavior

## Root Cause Analysis

### Problem
`QVariant(QString())` creates a QVariant containing an empty QString. In Qt5 vs Qt6, there's a **breaking change** in how `QVariant::isNull()` behaves:

**Qt 5.x behavior:**
- `QVariant(QString())` creates a **valid** variant of type QString
- `value.isNull()` returns **true** for QVariant containing null/empty QString
- Qt5 forwards `isNull()` calls to the contained type's `isNull()` method
- Since `QString().isNull()` returns true, `QVariant(QString()).isNull()` also returns true

**Qt 6.x behavior:**
- `QVariant(QString())` creates a **valid** variant of type QString
- `value.isNull()` returns **false** - Qt6 NO LONGER forwards `isNull()` to contained types
- A QVariant is only null if it contains no initialized value or contains a null pointer
- `QVariant(QString()).isNull()` returns false because the variant contains an initialized QString

**Official Qt Documentation:**
- [Qt 6 Core Changes](https://doc.qt.io/qt-6/qtcore-changes-qt6.html)
- [RFC: QVariant changes in Qt6](https://lists.qt-project.org/pipermail/development/2019-November/038002.html)
- [QVariant Class Documentation](https://doc.qt.io/qt-6/qvariant.html)

### Critical Code Path

In `variant_json.cpp` line 24-26:
```cpp
if (!value.isValid() || value.isNull()) {
  return QJsonValue();  // null
}
```

**The issue:** In Qt5, `QVariant(QString()).isNull()` returns `true` (forwarded to QString::isNull()), causing the function to return `QJsonValue()` (JSON null) instead of `QJsonValue(QString())` (JSON empty string "").

In Qt6, the same code returns false, so it proceeds to the type switch and correctly returns an empty string.

### Expected vs Actual

**Test expects:**
```cpp
QCOMPARE(variantToJson(QVariant(QString())), QJsonValue(QString()));
```
- Expected: `QJsonValue(QString())` → QJsonValue containing empty string ""
- Actual (Qt5): `QJsonValue()` → QJsonValue null (type mismatch!)
- Actual (Qt6): `QJsonValue(QString())` → QJsonValue containing empty string "" (correct!)

### Affected Types in Qt5

The following Qt types have `isNull()` methods and would be affected by Qt5's forwarding behavior:
- QString
- QByteArray
- QDate
- QTime
- QDateTime
- QUrl
- QBitArray
- QModelIndex
- QRegularExpression

All of these could incorrectly return JSON null in Qt5 when they contain "null" values but are valid QVariants.

## Resolution

root_cause: Qt5 QVariant::isNull() returns true for QVariant containing null/empty QString because Qt5 forwards isNull() to the contained type's isNull() method. Qt6 removed this forwarding behavior intentionally as it was deemed bugprone. The variantToJson() function checks value.isNull() early (line 24), causing Qt5 to return JSON null instead of empty string for empty QStrings and other "null-like" Qt types.

fix: Remove the `|| value.isNull()` check from line 24 of variant_json.cpp, rely only on `!value.isValid()` to detect truly uninitialized QVariants. This aligns with Qt6's intentional design change.

verification: (pending fix)

files_changed: []

## Proposed Fix

**Option 1 (REJECTED):** Check type before null check
```cpp
// Handle QString specially to preserve empty strings
if (value.userType() == QMetaType::QString) {
  return value.toString();
}

if (!value.isValid() || value.isNull()) {
  return QJsonValue();  // null
}
```
*Downside:* Would need to handle QString, QDate, QTime, QDateTime, QUrl, QByteArray, etc. - any Qt type with isNull() method. This duplicates logic and is fragile. Doesn't scale.

**Option 2 (RECOMMENDED):** Remove isNull() check entirely
```cpp
if (!value.isValid()) {
  return QJsonValue();  // null
}
// Remove || value.isNull() part entirely
```
*Rationale:*
- Qt6 developers intentionally removed isNull() forwarding because it was bugprone
- Only uninitialized QVariants should become JSON null
- If a QVariant contains an actual value (even an empty string), it should serialize that value
- This aligns with Qt6 behavior and modern Qt best practices
- Simpler and more maintainable - single condition
- Works consistently across Qt5 and Qt6

**Option 3 (ALTERNATIVE):** Qt5/Qt6 conditional compilation
```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  // Qt5: Only check isValid(), as isNull() has inconsistent forwarding behavior
  if (!value.isValid()) {
#else
  // Qt6: Can safely check both (though isNull() rarely true)
  if (!value.isValid() || value.isNull()) {
#endif
    return QJsonValue();  // null
  }
```
*Downside:* Adds complexity with conditional compilation. Since Qt6 behavior is objectively better (per Qt developers' assessment), just use that for both versions.

## Recommendation

**Use Option 2:** Remove `|| value.isNull()` entirely from line 24 of `src/probe/introspection/variant_json.cpp`

**Change:**
```cpp
// BEFORE (line 24):
if (!value.isValid() || value.isNull()) {

// AFTER:
if (!value.isValid()) {
```

This:
1. Fixes the Qt5 test failure for empty QString
2. Aligns with Qt6's intentional design change
3. Avoids bugprone behavior that Qt developers deemed worth breaking compatibility
4. Maintains correct behavior for truly invalid QVariants
5. Works consistently across all Qt5 and Qt6 versions
6. Simpler code with clearer semantics

## Additional Notes

**Why Qt made this change:** The old Qt5 behavior meant that code couldn't distinguish between:
- `QVariant()` - uninitialized variant (should be JSON null)
- `QVariant(QString())` - valid variant containing empty string (should be JSON "")
- `QVariant(QDate())` - valid variant containing null date (should be JSON object? null?)

Qt6 clarified this: a QVariant is null only if it's uninitialized. The contents of an initialized variant are a separate concern.

**Impact of this fix:** After removing `|| value.isNull()`:
- Empty strings will serialize to JSON "" (empty string)
- Null QDate/QTime will serialize to empty ISO date strings
- Null QUrl will serialize to empty string
- This matches Qt6 behavior and is more semantically correct

**Test coverage:** The failing test `testVariantToJsonString()` will pass after this change. Consider adding explicit tests for other affected types (QDate, QTime, QUrl) to prevent regressions.
