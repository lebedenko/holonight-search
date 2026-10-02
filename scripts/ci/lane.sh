#!/bin/sh
set -eu
lane=$1
case "$lane" in
  build-test) build_type=Debug ;;
  benchmark) build_type=Release ;;
  *) echo "Unknown lane: $lane" >&2; exit 2 ;;
esac
mkdir /work/source
cp -a /input/. /work/source/
cd /work/source
export HOME=/work/build/home LC_ALL=C.UTF-8 TZ=UTC
mkdir -p "$HOME"
python3 --version
git --version
cmake --version
ninja --version
c++ --version
pkg-config --modversion Qt6Core
python3 scripts/ci/test_launcher.py
cmake -S . -B build/verification -G Ninja -DCMAKE_BUILD_TYPE="$build_type" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON
cmake --build build/verification --parallel 2
case "$lane" in
  build-test)
    ctest --test-dir build/verification --output-on-failure --no-tests=error
    cmake --install build/verification --prefix /work/prefix
    cmake -S tests/package-consumer -B /work/consumer -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/work/prefix
    cmake --build /work/consumer --parallel 2
    /work/consumer/search-consumer
    ;;
  benchmark)
    python3 scripts/ci/prepare-tools.py
    export PATH=/work/tools/usr/bin:$PATH FZF_DEFAULT_OPTS= FZF_DEFAULT_COMMAND=
    fzf --version
    mkdir /work/benchmark
    benchmark_status=0
    build/verification/tests/holonight_search_benchmark 2000000 /work/benchmark/candidates.txt \
      > /work/benchmark/provider.log 2>&1 || benchmark_status=$?
    cat /work/benchmark/provider.log
    test "$benchmark_status" -eq 0 || exit "$benchmark_status"
    python3 scripts/ci/compare-fzf.py /work/benchmark/provider.log /work/benchmark/candidates.txt
    ;;
esac
