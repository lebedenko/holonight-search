#!/usr/bin/env python3
"""Compare fzf ingestion/filter timings with the existing representative benchmark."""
import json
from pathlib import Path
import re
import resource
import statistics
import subprocess
import sys
import time

provider = Path(sys.argv[1]).read_text()
candidates = Path(sys.argv[2])
if not re.search(r'^records=2000000 ', provider, re.MULTILINE):
    raise RuntimeError('The representative provider benchmark must index two million records.')
with candidates.open('rb') as source:
    if sum(1 for _ in source) != 2_000_000:
        raise RuntimeError('fzf must receive the same complete two-million-path candidate list.')
for query in ('lambda', 'lambda audit', 'audit lambda', 'document 42'):
    if f'query={query} hits=80 ' not in provider:
        raise RuntimeError(f'The representative provider query did not return its bounded hits: {query}')
    samples = []
    for _ in range(20):
        with candidates.open('rb') as source:
            started = time.perf_counter()
            subprocess.run(['fzf', '--filter', query], stdin=source, stdout=subprocess.DEVNULL, check=True)
            samples.append((time.perf_counter() - started) * 1000)
    print(json.dumps({'query': query, 'includes_candidate_ingestion': True,
                      'samples_ms': samples, 'p50_ms': statistics.median(samples),
                      'p95_ms': sorted(samples)[18]}), flush=True)
print(json.dumps({'fzf_peak_rss_kib': resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss}), flush=True)
