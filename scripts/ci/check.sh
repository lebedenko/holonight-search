#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
python3 scripts/test-quality.py
task test
task format-check
task tidy
reuse lint
