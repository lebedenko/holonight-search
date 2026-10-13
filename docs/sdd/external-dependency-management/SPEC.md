# External dependency management — holonight-search

Work package: EDM-011
Baseline: `26067775e2eac3d9a779b3114dfbddc96834c7de`
Umbrella audit: `38146d8f474317f3f8d72699f60e6d16a71dd165`
Implementation baseline: `993929cc717f3beaf99c60e01441dc3e86dfe7cf`
Umbrella handoff checkpoint: `d5fb7f5` (local; corrected Search declarations and Ready ledger)
Status: Implemented; local acceptance evidence is recorded below. Publication and pinning await authorization.

## Requirements

- Configuration and normal build commands shall never acquire dependencies or prepare source providers.
- `task deps` shall explicitly prepare supplied local providers and check installed external prerequisites.
- Internal providers shall resolve only from approved prefixes; external packages shall resolve from an explicit
  external prefix or normal system locations.
- Enabled dependency-dependent features shall require their dependencies; minimal profiles shall disable them explicitly.
- Where production components use Qt, discovery shall require >=6.11; private Qt components shall match
  the selected public version exactly.
- Where tests use GTest, consumers shall require installed GTest >=1.14 CONFIG and any GMock targets they use.
- Existing public APIs, component switches, install-tree consumers and acceptance coverage shall be preserved.

## Design

Adopt canonical `tooling/modules/holonight-search.json` and `.cmake`, then synchronize bundle 2.0.0 into this repository.
The copied metadata is owned by this standalone checkout. CMake performs authoritative discovery; shared probes
provide prerequisite diagnostics and record resolved versions/paths. Incompatible caches and local provider install
trees are archived before reuse. Package acquisition occurs only in explicit environment preparation. CI switches
build/runtime/release lanes together to qualified immutable images and runs acceptance offline after preparation.

Preserve existing dependency interfaces and component switches; adopt installed GTest CONFIG discovery where tests use it.

## Implementation and verification tasks

- [x] Confirm the exact baseline and canonical handoff before implementation.
- [x] Adopt declarations, prefix helpers, explicit preparation and compatibility signatures.
- [x] Apply the repository-specific discovery changes above.
- [x] Update Task, CI preparation/entrypoints, release lanes and contributor instructions.
- [x] Run declaration/CMake/profile consistency checks and offline missing-package regressions.
- [x] Run narrow regressions, clean acceptance, installed-package consumers and required quality checks.
- [x] Inspect complete logs; record commands, versions, features, warnings and manual checks.
- [x] Make a repository-local commit only after local verification; publish only when authorized.

## Search implementation

Search adopts bundle 2.0.0 and byte-identical corrected canonical JSON/CMake declarations.
Qt Core >=6.11 remains required. GTest >=1.14 CONFIG is required only when BUILD_TESTING is ON.
Unused Qt Test metadata and QML preset settings are removed. fzf stays in the existing test
profile; the benchmark uses the qualified image's installed binary. The retired acquisition
helper is removed; `scripts/ci/packages.json` preserves the historical 0.74.4-1 URL/checksum.
Package version 0.1.0, public C++ API and `HolonightSearch::Search` are unchanged.

There are no internal providers. `task deps PRESET=test` explicitly validates and records
installed prerequisites. Configure/build/test only validate discovery and compatible caches.
`HOLONIGHT_EXTERNAL_PREFIX` supports installed packages and paths with spaces; direct CMake
accepts `CMAKE_PREFIX_PATH`. Changed external identities preserve caches as `.previous-N`.

`task ci:prepare` runs `python3 scripts/ci/run.py --prepare`: acquire qualified images, take a
working-tree snapshot, validate test prerequisites offline, record versions/packages/external
state, then write a hash-validated manifest. `task ci` requires matching input and environment
hashes before starting containers. Changed identities archive `ci-prepared.previous-N`; matching
re-preparation preserves the previous manifest. Failed prerequisite validation cannot publish an accepted manifest.
The input identity covers dependency tooling, CMake/presets, lanes and launcher plus image identities.

Exact linux/amd64 identities:

- Arch full: `ghcr.io/lebedenko/holonight-deps@sha256:37ca051c39d1688d6413c47d94b8c0f221152dac6981ec67d155f2c43ea17b40`
- REUSE: `docker.io/fsfe/reuse@sha256:85462a75c0f8efda09ddd190b92816b70e7662577c8427429e11e1b9f25a992e`

Acceptance runs with caller UID/GID, `--pull=never --network=none`, read-only source/preparation
mounts, private mode-0700 runtime directory, disposable writable copies and JOBS=2 by default.
The five hosted matrix jobs each explicitly prepare, run their lane, and upload logs/results.

Build-test retains Debug tests and an installed consumer. Benchmark retains two million records,
the complete shared candidate list, four queries, 80 provider hits and 20 samples for each query.
Release uses BUILD_TESTING=OFF, verifies staging excludes tests/benchmark executables, and runs
an installed consumer with spaces in both prefix and build paths. Static checks build compile
commands for all four C++ units including the consumer, then apply canonical format and strict
tidy. Licensing runs the pinned REUSE environment.

Behavior-preserving C++ cleanup is committed separately: designated initializers, ranges algorithms,
explicit existing conversions, boolean tests and private member naming. Targeted NOLINT comments
preserve the public enum representation and the established scoring/search control flow; no ranking
constants, candidate ordering, query semantics or test assertions change.

## Verification — 2026-10-13

Acceptance details and environment hashes: [acceptance.json](verification/acceptance.json) and
[environment.json](verification/environment.json). Complete logs remain in the ignored local
build/ci directory recorded in the evidence; hosted runs upload equivalent artifacts.

Clean preparation `python3 scripts/ci/run.py --prepare` passed at
`build/ci/20261013T134822Z-n4lhsir8`; clean five-lane `python3 scripts/ci/run.py` passed at
`build/ci/20261013T134825Z-tnsa2kj6`. All six provider tests, Debug/Release installed consumers,
eight staged files, four strict-tidy units, canonical formatting and REUSE passed.
Environment: GCC 16.2.1, CMake 4.4.3, Ninja 1.13.2, Python 3.14.7, Qt 6.11.2,
GTest 1.18.0-1, LLVM 22.1.8, fzf 0.74.4-1 (a140afeb), REUSE 6.2.0, Docker 29.9.0.

Benchmark: indexing 2300 ms; provider RSS/HWM 1,368,604 KiB; fzf peak RSS 479,828 KiB.
All queries returned 80 provider hits and all 80 fzf samples completed:

| Query | Provider warm p50/p95 ms | fzf ingestion/filter p50/p95 ms |
| --- | --- | --- |
| lambda | 4 / 4 | 127.31 / 187.14 |
| lambda audit | 4 / 5 | 137.43 / 186.66 |
| audit lambda | 4 / 4 | 171.44 / 182.08 |
| document 42 | 66 / 73 | 436.27 / 453.96 |

The report records a dirty pre-commit working-tree acceptance, including new source inputs;
implementation hashes identify the verified files independently of later commit boundaries.
Complete logs were inspected; build/consumer logs contain no actionable compiler/CMake warnings.
The launcher regression logs intentionally show failed fake lanes to verify failure propagation.

Documentation/evidence closure licensing also passed at `build/ci/20261013T135726Z-8a8npoly`.

Focused commands:

- `python3 scripts/ci/test_launcher.py`: 8 pass (snapshot/modes/symlinks/deletions, missing/stale/tampered
  preparation, incompatible archival, rootless/offline runtime arguments, every-lane failure propagation).
- `python3 scripts/ci/test_dependencies.py`: 12 pass (missing/old Qt and GTest, testing OFF without GTest,
  missing fzf, ordinary commands with no acquisition/preparation, installed discovery and prefixes with spaces,
  compatible cache reuse and changed identity archival). Repeated inside qualified build-test acceptance.
- Umbrella `python3 tooling/test_external.py` and `python3 tooling/test_tooling.py`: 21 + 11 pass.
- Umbrella `python3 tooling/sync.py --check --module holonight-search` and
  `python3 tooling/generate-install-profile.py --check`: pass; JSON/CMake declarations compare byte-identical.
- Workflow YAML parsing, `sh -n scripts/ci/lane.sh` and `git diff --check`: pass.

Qualification is for the pinned Arch environment. Host LLVM 23 has an additional trailing-comma
diagnostic; strict acceptance uses qualified LLVM 22.1.8. System-header diagnostic counts are
suppressed by canonical tidy; all owned diagnostics are resolved or narrowly documented above.
No Debian qualification, downstream adoption, ranking changes, new fuzzing or manual ecosystem
integration is claimed. Benchmark timings are observations, with no performance threshold;
provider warm-query times and fzf ingestion/filter times are separate measures.

No remote publication, CI query or umbrella pin change is part of this local handoff.
