# Local CI rehearsal

Baseline: c9751c6f4a0751192c0be5d83aeb9528993e68e4. Umbrella package CI-006.

Add missing hosted validation with two independent lanes in the pinned Qt6/compiler
image. Debug/Ninja builds all targets, runs provider CTest with no-tests=error and
builds/runs the existing separate installed-package consumer. Release/Ninja builds
fresh and runs the representative two-million-path benchmark plus checksum-pinned
fzf 0.74.4 against the identical generated list, four existing queries and 20 samples.
Report provider indexing/RSS separately from warm query times; fzf timings include
candidate ingestion and are not equivalent to the warm-index measurements.
Verify record/list counts and bounded query hits. Performance measurements are
reported rather than gated against machine-specific historical thresholds.
No external provider is needed; do not change search behavior or development tasks.

Snapshot tracked edits and non-ignored new inputs, omit deleted/ignored files,
preserve modes and symlinks, and report untracked inputs to add before pushing.
Input is read-only; writable source/build/provider trees are disposable. Save full
logs, revision/dirty status, image identity, tool versions and lane results in build/ci.
Docker is preferred; absent Docker falls back to Podman with rootless UID/GID mapping.
Unavailable or failed required checks return nonzero. Pushes and pin updates are
excluded. Local acceptance passed on 2026-10-03.


Verification: both clean lanes passed in build/ci/20261002T223423Z-6dh09xr2/.
Debug passed six provider CTest cases and the separate installed consumer. Release
indexed two million records in 2,320 ms at 1,367,516 KiB peak RSS; provider p50/p95
query times were 4/5, 5/5, 5/5 and 75/78 ms. The identical-list fzf comparison
completed all 80 samples at 478,236 KiB peak RSS; full sample values are in the log.
Four launcher regressions passed; hashes/modes/timestamps confirmed 119 source and
development-build files stayed unchanged. Complete logs and final diff were reviewed.
No source behavior changed. Real Podman execution remains unverified because it is
not installed on this host.
