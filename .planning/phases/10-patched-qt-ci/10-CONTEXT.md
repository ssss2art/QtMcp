# Phase 10: Patched Qt 5.15.1 CI - Context

**Gathered:** 2026-02-02
**Status:** Ready for planning

<domain>
## Phase Boundary

CI builds and caches a custom-patched Qt 5.15.1 from source, then builds the probe against it on both Windows and Linux. This is isolated from the standard matrix CI (Phase 9). Restore, other Qt versions from source, and release automation are separate phases.

</domain>

<decisions>
## Implementation Decisions

### Patch delivery
- Patch files committed to the repo (version-controlled, reviewable in PRs)
- Format: git diff / .patch files
- Multiple patches covering different areas of Qt
- Directory structure: version-organized (e.g., `.ci/patches/5.15.1/`) to support potential future versions

### Build configuration
- Qt source obtained from official Qt archive (qt-everywhere-src-5.15.1.tar.xz from download.qt.io)
- Shared (dynamic) libraries, not static
- Which Qt modules to build and specific configure flags: **delegate to researcher** — analyze what the probe links against and recommend minimal configure flags

### Cache strategy
- Cache key includes: hash of patch files + runner OS + configure flags
- No scheduled cache warming — build on demand (first cold-cache run takes 30-60 min, subsequent runs hit cache)
- Cache size acceptable (1-3 GB within GitHub Actions 10 GB repo limit)
- Qt build step extracted as a **reusable composite action** (`.github/actions/build-qt/`) so Phase 11 release workflow can reuse it

### Workflow structure
- **Separate workflow** file (not part of existing CI matrix) — isolated triggers, timeouts, and status
- Trigger: **push to main only** (skip PRs to save CI minutes)
- Release gate: **soft gate** — release can proceed without patched Qt binaries (they're bonus artifacts)
- Timeout: **120 minutes** per job (headroom for cold-cache Windows MSVC builds)

### Claude's Discretion
- Exact Qt modules to build (after research)
- Configure flags (after research)
- Cache compression and cleanup approach
- Composite action input/output interface design
- Patch application order and error handling

</decisions>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 10-patched-qt-ci*
*Context gathered: 2026-02-02*
