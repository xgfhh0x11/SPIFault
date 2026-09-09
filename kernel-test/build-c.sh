#!/usr/bin/env bash
# build-c.sh — build SPIFault's C data-model library + examples via the
# repo's own Makefile.
#
# SPIFault is a SIMULATION-ONLY framework (see kernel-test/README.md).
# This script builds the userspace static libraries under src/ and the
# examples/basic_example.c smoke test. There is no kernel module, no
# device driver, and nothing here touches real hardware.
#
# Intended to run under WSL2 Ubuntu-22.04 (or any Linux/macOS shell with
# gcc). Will not work in plain Windows Git Bash / PowerShell — there is
# no MSVC-compatible build path for this Makefile (POSIX headers only).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "== SPIFault build-c.sh =="
echo "repo root: $REPO_ROOT"

if ! command -v gcc >/dev/null 2>&1; then
    echo ""
    echo "ERROR: gcc not found on PATH."
    echo "This repo's Makefile needs a POSIX toolchain (gcc, make, ar)."
    echo "Under WSL2 Ubuntu-22.04, install with:"
    echo ""
    echo "    sudo apt-get update && sudo apt-get install -y build-essential"
    echo ""
    exit 1
fi

if ! command -v make >/dev/null 2>&1; then
    echo ""
    echo "ERROR: make not found on PATH."
    echo "Under WSL2 Ubuntu-22.04, install with:"
    echo ""
    echo "    sudo apt-get update && sudo apt-get install -y build-essential"
    echo ""
    exit 1
fi

echo "gcc:  $(gcc --version | head -1)"
echo "make: $(make --version | head -1)"
echo ""

cd "$REPO_ROOT"

echo "== make clean =="
make clean

echo ""
echo "== make all (libspi_core.a, libinjection.a, libglitch.a, libspi_framework.a, examples) =="
make all

echo ""
echo "== artifacts (build/) =="
ls -la build/

# Run the smoke example directly rather than via `make test`: `make test`
# treats any non-zero exit from basic_example as a build failure (`make`
# aborts with "Error 1"), which conflates "did it compile" with "did the
# example's own control flow succeed". We want build-c.sh to report both,
# clearly, without one masking the other.
echo ""
echo "== running build/basic_example =="
RUN_STATUS=0
( cd build && ./basic_example ) || RUN_STATUS=$?

echo ""
if [ "$RUN_STATUS" -eq 0 ]; then
    echo "basic_example: exited 0"
else
    echo "basic_example: exited $RUN_STATUS (non-zero)"
    echo ""
    echo "NOTE (discovered by this script, not a build-infra bug): examples/basic_example.c"
    echo "calls spi_framework_start() at step 2 before it registers any glitch fault"
    echo "parameters (that happens later, at step 9, via glitch_engine_add_fault_parameters)."
    echo "glitch_engine_start_glitching() (src/glitch_engine.c) requires"
    echo "engine->glitch_count > 0 and returns false otherwise (\"No glitch parameters"
    echo "configured\"), so spi_framework_start() fails and main() returns 1 immediately"
    echo "-- steps 3-12 of the example never execute. This is a pre-existing ordering bug"
    echo "in the example, not something this test infra introduced; it was never caught"
    echo "before because the C side had never been compiled (see repo review)."
fi

echo ""
echo "build-c.sh: OK (compile + link succeeded; basic_example exit=$RUN_STATUS, see note above if non-zero)"
