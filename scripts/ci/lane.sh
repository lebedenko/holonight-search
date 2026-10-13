#!/bin/sh
set -eu
lane=$1
mkdir /work/source
cp -a /input/. /work/source/
cd /work/source
export HOME=/work/build/home LC_ALL=C.UTF-8 TZ=UTC
export QT_QPA_PLATFORM=offscreen XDG_RUNTIME_DIR=/work/runtime QT_FORCE_STDERR_LOGGING=1
export XDG_DATA_HOME=/work/data XDG_CONFIG_HOME=/work/config XDG_CACHE_HOME=/work/cache
mkdir -p "$HOME" "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME"
mkdir -m 700 "$XDG_RUNTIME_DIR"
if [ "$lane" = licensing ]; then
  reuse --version
  reuse --no-multiprocessing lint
  exit
fi
if [ "$lane" = prepare ]; then
  task deps PRESET=test
  python3 tooling/external.py check --module tooling/module.json --scope build --scope tool --scope test
  pacman -Q > /prepared/packages.txt
  c++ --version > /prepared/compiler.txt
  cmake --version > /prepared/cmake.txt
  { python3 --version; ninja --version; clang++ --version; clang-tidy --version;
    pkg-config --modversion Qt6Core; fzf --version;
    pacman -Q gtest fzf; } > /prepared/versions.txt
  cp build/deps/external-state.json /prepared/external-state.json
  exit
fi
cat /prepared/compiler.txt /prepared/cmake.txt /prepared/versions.txt
cp /prepared/packages.txt /prepared/compiler.txt /prepared/cmake.txt /prepared/versions.txt /output/
case "$lane" in
  build-test)
    python3 scripts/ci/test_launcher.py
    python3 scripts/ci/test_dependencies.py
    # Keep the established Debug provider-test configuration.
    cmake -S . -B build/verification -G Ninja -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON
    cmake --build build/verification --parallel "${JOBS:-2}"
    ctest --test-dir build/verification --output-on-failure --no-tests=error
    cat build/verification/Testing/Temporary/LastTest.log
    cp -a build/verification/Testing /output/
    cmake --install build/verification --prefix '/work/debug prefix'
    cmake -S tests/package-consumer -B '/work/debug consumer' -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug '-DCMAKE_PREFIX_PATH=/work/debug prefix'
    cmake --build '/work/debug consumer' --parallel "${JOBS:-2}"
    '/work/debug consumer/search-consumer'
    ;;
  benchmark)
    cmake -S . -B build/benchmark -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
    cmake --build build/benchmark --parallel "${JOBS:-2}"
    export FZF_DEFAULT_OPTS= FZF_DEFAULT_COMMAND=
    fzf --version
    mkdir /work/benchmark
    benchmark_status=0
    build/benchmark/tests/holonight_search_benchmark 2000000 /work/benchmark/candidates.txt \
      > /work/benchmark/provider.log 2>&1 || benchmark_status=$?
    cat /work/benchmark/provider.log
    cp /work/benchmark/provider.log /output/
    test "$benchmark_status" -eq 0 || exit "$benchmark_status"
    python3 scripts/ci/compare-fzf.py /work/benchmark/provider.log /work/benchmark/candidates.txt > /output/fzf.jsonl
    cat /output/fzf.jsonl
    ;;
  release)
    python3 tooling/workflow.py build --preset release
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
    cp -a /work/package /output/
    ;;
  static-checks)
    git init -q
    git add .
    python3 tooling/workflow.py build --preset test
    cmake --install build/test --prefix '/work/static prefix'
    cmake -S tests/package-consumer -B build/consumer -G Ninja \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON '-DCMAKE_PREFIX_PATH=/work/static prefix'
    cmake --build build/consumer --parallel "${JOBS:-2}"
    build/consumer/search-consumer
    python3 tooling/workflow.py format-check
    python3 tooling/workflow.py tidy --scope all
    ;;
  *) echo "Unknown lane: $lane" >&2; exit 2 ;;
esac
