# Shared search engine

Status: Implementation in progress. The [umbrella initiative](../../../../docs/initiatives/shared-search-engine/README.md) defines the cross-repository contract.

## Requirements

- The library shall accept batches of stable IDs and named weighted text fields from any product source.
- The library shall atomically replace a source snapshot and allow append-only batches while source collection proceeds.
- A query shall match fuzzy characters within each whitespace term and require every term in any order; uppercase characters enable case-sensitive matching.
- Ranking shall use boundary and consecutive bonuses, explicit field weights and record boosts. The path profile shall prefer filename hits.
- Search shall return bounded IDs, scores, and UTF-16 highlight offsets and shall discard results when cancelled.
- The installed CMake package shall work from a separate consumer project.
- A two-million-path Release benchmark shall compare the same candidate list and queries with fzf, and record memory and index time separately from warm query time.

## Design

The public `Index` is independent of product code. It publishes immutable record batches under a short write lock; search copies batch handles under a read lock, then ranks without holding the lock. It uses a compact character-presence mask to reject impossible records, dynamic programming for fuzzy terms, a fast contiguous boundary match, and per-worker bounded heaps. Highlight reconstruction runs only for the selected results. A source filter lets a consumer keep directory and file records separate.

No filesystem traversal, package database, launcher rule, persistent index, typo correction, or additional fzf operator belongs here. The public API deliberately leaves source policy to consumers.

## Tasks

| ID | Task | State |
|---|---|---|
| P1 | Public API, matcher, snapshots, cancellation | Done |
| P2 | Relevance and installed consumer tests | Done |
| P3 | Representative fzf comparison and performance acceptance | Done for provider query path; Files UI latency pending |
| P4 | Local acceptance and publication handoff | Planned |

## Verification

On 2026-09-30, Release CTest passed six provider tests. A staged install in `/tmp/holonight-search-prefix` configured, built, and ran the separate `tests/package-consumer` project. `reuse --no-multiprocessing lint` passed. The default multiprocessing mode failed because the sandbox denied its worker socket; the single-process mode checks the same files.

The deterministic two-million-path generator wrote the candidate list to `/tmp/holonight-search-2m-final.txt`. On this machine (Intel Core i9-14900HX, Qt 6.11.2), the provider constructed the index in 2.22 s with 1,374,960 KiB resident memory. Twenty warm samples per query in Release yielded:

| Query | Provider p50 / p95 | fzf 0.74.4 p50 / p95 |
|---|---:|---:|
| `lambda` | 4 / 4 ms | 148 / 189 ms |
| `lambda audit` | 4 / 5 ms | 135 / 191 ms |
| `audit lambda` | 4 / 5 ms | 137 / 185 ms |
| `document 42` | 69 / 71 ms | 407 / 470 ms |

fzf used `--filter` and reread the identical two-million-line file for every sample; its timings include ingestion and are **not** a like-for-like warm-index comparison. One fzf run used 463,652 KiB peak RSS. The provider's larger memory footprint remains a trade-off of the generic field index. These measurements are provider query duration, not Files final-keystroke to stable UI results. Cold filesystem traversal, scan progress, and first-result latency require the Files consumer and are pending.
