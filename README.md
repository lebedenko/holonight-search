# HoloNight Search

For current build, test, container acceptance and provider workflows, see [DEVELOPMENT.md](DEVELOPMENT.md).

An in-memory Qt Core fuzzy search library for HoloNight applications. Sources supply stable record IDs and named text fields with explicit weights and boosts. The library does not collect files or control a UI.

```cpp
#include <holonight/search/index.h>

HolonightSearch::Index index;
index.replace("files", {{.id = "1", .fields = {{"name", "lambda.cpp", 100}, {"path", "src/lambda.cpp", 20}}}});
auto hits = index.search("lambda", {.limit = 80, .profile = HolonightSearch::Profile::Path});
```

Build with CMake, install to a prefix, then use `find_package(HolonightSearch CONFIG REQUIRED)` and link `HolonightSearch::Search`. The current syntax is fuzzy matching within whitespace-separated AND terms, with smart case. Other fzf operators are deferred.

`Index::append` publishes immutable batches while a source scans; `replace` swaps a complete source snapshot and `remove` drops it. IDs must be unique within a source. `Options::source` limits a query to one source, which can represent Files' directory or file kind. Field weights and record boosts are caller supplied; the path profile adds a filename preference when a field is named `name`. A cancelled query returns no results.
