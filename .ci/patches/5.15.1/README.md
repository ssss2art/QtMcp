# Qt 5.15.1 Patches

This directory contains patch files applied to the official Qt 5.15.1 source
tree during CI builds. The reusable composite action at
`.github/actions/build-qt/action.yml` applies these patches automatically.

## Naming Convention

Patches are named with a numeric prefix for deterministic ordering:

```
001-description-of-change.patch
002-another-fix.patch
003-yet-another-change.patch
```

Patches are applied in **sorted filename order** (lexicographic), so the
numeric prefix controls the application sequence.

## Patch Format

- Patches must be in **git diff** format (generated with `git diff` or
  `git format-patch`).
- Patches are applied with `git apply --verbose` inside the extracted Qt
  source tree (after `git init && git add -A && git commit`).
- The standard `-p1` strip level is used (strips the leading `a/` and `b/`
  prefixes from git-generated diffs).
- Each patch should include a descriptive header explaining what it changes
  and why.

## Creating a Patch

```bash
# From a modified Qt source tree that has been git-initialized:
git diff > 001-description.patch

# Or for staged changes:
git diff --cached > 001-description.patch

# Or using format-patch for a single commit:
git format-patch -1 --stdout > 001-description.patch
```

## Cache Invalidation

The CI cache key for built Qt includes a hash of all files in this directory:

```yaml
hashFiles('.ci/patches/5.15.1/**')
```

Any change to patches -- adding, removing, or editing a `.patch` file --
automatically invalidates the cached Qt build, triggering a fresh
build-from-source on the next CI run. No manual cache busting is needed.

## Current Patches

None yet. Add `.patch` files here as needed.
