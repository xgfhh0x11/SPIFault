#!/usr/bin/env bash
# verify.sh — run SPIFault's pytest suite, then confirm the campaign report
# outputs (JSON + Markdown) produced by run-campaign.sh are well-formed.
#
# This is the "did the fixes + infra actually work" check: it does not
# build or touch the C library (see build-c.sh / asan-check.sh for that).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUT_DIR="$SCRIPT_DIR/output"

echo "== SPIFault verify.sh =="
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
    exit 1
fi

echo "python: $("$PYTHON" --version)"

if ! "$PYTHON" -c "import pytest, yaml" >/dev/null 2>&1; then
    echo ""
    echo "Installing test deps from tests/requirements.txt ..."
    "$PYTHON" -m pip install -q -r "$REPO_ROOT/tests/requirements.txt"
fi

cd "$REPO_ROOT"

echo ""
echo "== 1/2: python -m pytest -q -p no:cacheprovider =="
PYTEST_STATUS=0
"$PYTHON" -m pytest -q -p no:cacheprovider || PYTEST_STATUS=$?

echo ""
echo "== 2/2: campaign report well-formedness =="

if [ ! -d "$OUT_DIR" ] || [ -z "$(ls -A "$OUT_DIR" 2>/dev/null || true)" ]; then
    echo "no reports in $OUT_DIR yet — running run-campaign.sh first"
    "$SCRIPT_DIR/run-campaign.sh" || true
fi

REPORT_STATUS=0
"$PYTHON" - "$OUT_DIR" <<'PYEOF' || REPORT_STATUS=$?
import glob
import json
import os
import sys

out_dir = sys.argv[1]
json_files = sorted(glob.glob(os.path.join(out_dir, "*.json")))

if not json_files:
    print(f"  FAIL: no *.json reports found under {out_dir}")
    sys.exit(1)

required_keys = {
    "campaign_id", "backend", "success_rate",
    "within_expected", "outcome_counts",
}

bad = 0
for jf in json_files:
    base = os.path.splitext(jf)[0]
    md = base + ".md"
    name = os.path.basename(jf)

    try:
        with open(jf, encoding="utf-8") as f:
            doc = json.load(f)
    except Exception as e:
        print(f"  FAIL: {name}: not valid JSON ({e})")
        bad += 1
        continue

    missing = required_keys - set(doc.keys())
    if missing:
        print(f"  FAIL: {name}: missing keys {sorted(missing)}")
        bad += 1
        continue

    if not (0.0 <= doc["success_rate"] <= 1.0):
        print(f"  FAIL: {name}: success_rate out of [0,1]: {doc['success_rate']}")
        bad += 1
        continue

    if not os.path.exists(md):
        print(f"  FAIL: {name}: companion Markdown report missing ({md})")
        bad += 1
        continue

    print(f"  OK: {name}: id={doc['campaign_id']!r} backend={doc['backend']!r} "
          f"success_rate={doc['success_rate']:.3f} within_expected={doc['within_expected']}")

if bad:
    print(f"\n  {bad}/{len(json_files)} report(s) malformed")
    sys.exit(1)

print(f"\n  {len(json_files)}/{len(json_files)} report(s) well-formed")
PYEOF

echo ""
if [ "$PYTEST_STATUS" -eq 0 ] && [ "$REPORT_STATUS" -eq 0 ]; then
    echo "verify.sh: PASS (pytest exit=$PYTEST_STATUS, report-check exit=$REPORT_STATUS)"
    exit 0
else
    echo "verify.sh: FAIL (pytest exit=$PYTEST_STATUS, report-check exit=$REPORT_STATUS)"
    exit 1
fi
