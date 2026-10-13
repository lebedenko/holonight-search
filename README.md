# HoloNight Search

An in-memory Qt Core fuzzy search library for HoloNight applications. Sources supply stable record IDs and named text fields with explicit weights and boosts. The library does not collect files or control a UI.

```cpp
#include <holonight/search/index.h>

HolonightSearch::Index index;
index.replace("files", {{.id = "1", .fields = {{"name", "lambda.cpp", 100}, {"path", "src/lambda.cpp", 20}}}});
auto hits = index.search("lambda", {.limit = 80, .profile = HolonightSearch::Profile::Path});
```

Build with CMake, install to a prefix, then use `find_package(HolonightSearch CONFIG REQUIRED)` and link `HolonightSearch::Search`. The current syntax is fuzzy matching within whitespace-separated AND terms, with smart case. Other fzf operators are deferred.

`Index::append` publishes immutable batches while a source scans; `replace` swaps a complete source snapshot and `remove` drops it. IDs must be unique within a source. `Options::source` limits a query to one source, which can represent Files' directory or file kind. Field weights and record boosts are caller supplied; the path profile adds a filename preference when a field is named `name`. A cancelled query returns no results.

## Standalone developer tooling

See [tooling/README.md](tooling/README.md) for presets, local dependency overrides, editor refresh,
`task tooling:doctor`, and the independent Serena project.


## Local CI rehearsal

Run `task ci:prepare` explicitly to acquire the two qualified linux/amd64 images and
record installed prerequisite evidence, then run `task ci`. Docker must be accessible;
Podman is used when Docker is absent. Preparation needs registry access. Acceptance uses
cached images with `--pull=never --network=none`, caller UID/GID, read-only snapshots,
private runtime directories and two build jobs by default (`JOBS` overrides this).

Search has no internal source providers. `task deps PRESET=test` validates installed
Qt Core >=6.11, GTest >=1.14 CONFIG and fzf, and records prerequisite evidence;
`task deps` with the debug/release preset excludes test prerequisites. Ordinary
configure/build/test commands only validate and never acquire packages or prepare sources.
Use `HOLONIGHT_EXTERNAL_PREFIX` for external installations, including paths with spaces;
compatible caches are reused and incompatible trees are preserved as `.previous-N`.

Five lanes cover Debug provider tests and an installed consumer, the Release benchmark,
Release staging and an installed consumer, canonical format/strict tidy across all four
owned C++ units, and REUSE. The benchmark preserves two million paths, four queries,
80 provider hits and 20 samples per query. fzf is installed in the qualified image;
its ingestion/filter timings differ from warm provider queries. No performance threshold
is imposed. Historical fzf package identity remains in `scripts/ci/packages.json`.

Tracked edits and non-ignored new files enter snapshots, including executable modes and
symlinks; deleted and ignored files are omitted. Every lane uses a fresh build tree.
Logs and exported results live in ignored `build/ci/`; prepared versions, package list,
external discovery and a hash-validated manifest live in `build/ci-prepared/`. Missing,
stale or tampered evidence rejects acceptance before containers start. Rerun preparation
when its inputs change; incompatible evidence is archived as `ci-prepared.previous-N`.
Run focused regressions with `python3 scripts/ci/test_launcher.py` and
`python3 scripts/ci/test_dependencies.py`. Exact identities and acceptance evidence are in
[the local SPEC](docs/sdd/external-dependency-management/SPEC.md).
