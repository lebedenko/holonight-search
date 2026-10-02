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

Run `task ci` with Git, Python 3, Task and an accessible Docker daemon; Podman is
used when Docker is absent. The pinned linux/amd64 image needs an amd64 host or
emulation, registry access, and Arch archive access for checksum-pinned fzf 0.74.4.
Local and hosted CI share isolated Debug provider/installed-consumer checks and a
Release two-million-path benchmark with the existing four queries and 20 samples.
fzf filter timings include candidate ingestion; provider timings measure warm queries.
Memory and indexing measurements remain separate from query measurements.

Tracked edits and non-ignored new files enter read-only snapshots; untracked inputs
are reported to add before pushing. Every lane uses a disposable writable copy and
fresh build tree. Existing development builds stay untouched; only container layers
may be reused. Complete logs, revision/dirty state, image identity, tool versions
and results go to ignored `build/ci/`. Required failures return nonzero and print
complete failure logs. Run launcher regressions with `python3 scripts/ci/test_launcher.py`.
Publication, releases and artifact uploads remain remote operations.
