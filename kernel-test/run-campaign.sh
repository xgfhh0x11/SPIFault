#!/usr/bin/env bash
# run-campaign.sh — run SPIFault's Python FCDF simulator end-to-end on the
# shipped campaign(s).
#
# This drives the ONLY implemented backend: the in-process deterministic
# `simulator` (fcdf/simulator.py, registered under name "simulator").
# It does not touch real hardware. Steps, per campaign:
#   1. validate  — fcdf.loader.load_campaign() parses + schema-validates
#                  the YAML (fcdf/schema.py's validate()).
#   2. run       — fcdf.runner.run_campaign() resolves the "simulator"
#                  backend and executes the trial(s) via
#                  `python -m fcdf <campaign.yaml> --out <dir>`.
#   3. report    — the runner writes <output>/<campaign-id>.json and
#                  <output>/<campaign-id>.md.
#
# Usage:
#   ./run-campaign.sh                       # run all campaigns/*.yaml
#   ./run-campaign.sh campaigns/voltage_glitch_basic.yaml   # run one
#
# Works on Windows (Git Bash/PowerShell-hosted bash) or WSL2/Linux —
# the Python side has no POSIX-only dependencies.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUT_DIR="$SCRIPT_DIR/output"

echo "== SPIFault run-campaign.sh =="
echo "repo root: $REPO_ROOT"

PYTHON=""
for candidate in python3 python; do
    if command -v "$candidate" >/dev/null 2>&1; then
        PYTHON="$candidate"
        break
    fi
done

if [ -z "$PYTHON" ]; then
    echo ""
    echo "ERROR: no python3/python found on PATH."
    echo "Install Python 3.10+ (Windows: https://python.org, WSL: sudo apt-get install -y python3 python3-pip)."
    exit 1
fi

echo "python: $("$PYTHON" --version)"

if ! "$PYTHON" -c "import yaml" >/dev/null 2>&1; then
    echo ""
    echo "Installing FCDF's runtime deps (PyYAML) from tests/requirements.txt ..."
    "$PYTHON" -m pip install -q -r "$REPO_ROOT/tests/requirements.txt"
fi

cd "$REPO_ROOT"

if [ "$#" -gt 0 ]; then
    CAMPAIGNS=("$@")
else
    CAMPAIGNS=(campaigns/*.yaml)
fi

mkdir -p "$OUT_DIR"

echo ""
echo "campaigns to run: ${CAMPAIGNS[*]}"
echo "output dir: $OUT_DIR"
echo ""

FAILED=0
for campaign in "${CAMPAIGNS[@]}"; do
    echo "---- validate: $campaign ----"
    "$PYTHON" -c "
import sys
from fcdf.loader import load_campaign
c = load_campaign(sys.argv[1])
print(f'  schema OK: id={c.campaign.id!r} backend={c.target.device!r} '
      f'fault={c.fault.model.value!r} reps={c.trial.repetitions}')
" "$campaign"

    echo "---- run (simulator backend): $campaign ----"
    if ! "$PYTHON" -m fcdf "$campaign" --out "$OUT_DIR"; then
        echo "  !! campaign run reported outside expected band (non-zero exit) — see report for details"
        FAILED=1
    fi
    echo ""
done

echo "== reports written to: $OUT_DIR =="
ls -la "$OUT_DIR"

echo ""
echo "run-campaign.sh: done (exit=$FAILED)"
exit "$FAILED"
