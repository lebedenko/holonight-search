#!/usr/bin/env bash
set -euo pipefail
mkdir -p /work/output
    task build:release
    DESTDIR=/work/package cmake --install build/release
    cmake --install build/release --prefix '/work/external prefix'
    cmake -S tests/package-consumer -B '/work/consumer build' -G Ninja \
      -DCMAKE_BUILD_TYPE=Release '-DCMAKE_PREFIX_PATH=/work/external prefix'
    cmake --build '/work/consumer build' --parallel "${JOBS:-2}"
    '/work/consumer build/search-consumer'
    python3 - <<'CHECK'
from pathlib import Path
files = [str(p.relative_to('/work/package')) for p in Path('/work/package').rglob('*') if p.is_file()]
assert any(p.endswith('libholonight_search.a') for p in files), files
assert not any('test' in p or 'benchmark' in p or p.startswith('usr/bin/') for p in files), files
print('Release staging:', *files, sep='\n')
CHECK
    cp -a /work/package /work/output/

    cmake -S . -B build/benchmark -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
    cmake --build build/benchmark --parallel "${JOBS:-2}"
    export FZF_DEFAULT_OPTS= FZF_DEFAULT_COMMAND=
    fzf --version
    mkdir /work/benchmark
    benchmark_status=0
    build/benchmark/tests/holonight_search_benchmark 2000000 /work/benchmark/candidates.txt \
      > /work/benchmark/provider.log 2>&1 || benchmark_status=$?
    cat /work/benchmark/provider.log
    cp /work/benchmark/provider.log /work/output/
    test "$benchmark_status" -eq 0 || exit "$benchmark_status"
    python3 scripts/ci/compare-fzf.py /work/benchmark/provider.log /work/benchmark/candidates.txt > /work/output/fzf.jsonl
    cat /work/output/fzf.jsonl
