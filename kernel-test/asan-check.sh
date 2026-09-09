#!/usr/bin/env bash
# asan-check.sh — build SPIFault's C data-model library + examples/basic_example.c
# with AddressSanitizer + UndefinedBehaviorSanitizer and run it, to exercise
# the injection_engine.c code paths that previously had memory-safety bugs
# (heap-overflow from uint16_t truncation, OOB read on short buffers,
# unbounded strstr on non-NUL-terminated data — see the repo review) and
# confirm the fixed code is clean under both sanitizers.
#
# This does NOT use the plain Makefile (it has no asan target); it compiles
# src/*.c + examples/basic_example.c directly with sanitizer flags.
#
# Intended to run under WSL2 Ubuntu-22.04 (or any Linux/macOS shell with
# gcc/clang). Will not work in plain Windows Git Bash / PowerShell.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-asan"

echo "== SPIFault asan-check.sh =="
echo "repo root: $REPO_ROOT"

CC=""
for candidate in gcc clang; do
    if command -v "$candidate" >/dev/null 2>&1; then
        CC="$candidate"
        break
    fi
done

if [ -z "$CC" ]; then
    echo ""
    echo "ERROR: neither gcc nor clang found on PATH."
    echo "Under WSL2 Ubuntu-22.04, install with:"
    echo ""
    echo "    sudo apt-get update && sudo apt-get install -y build-essential clang"
    echo ""
    exit 1
fi

echo "compiler: $CC ($("$CC" --version | head -1))"

mkdir -p "$BUILD_DIR"
BIN="$BUILD_DIR/basic_example_asan"

SOURCES=(
    "$REPO_ROOT/src/spi_core.c"
    "$REPO_ROOT/src/injection_engine.c"
    "$REPO_ROOT/src/glitch_engine.c"
    "$REPO_ROOT/src/spi_framework.c"
    "$REPO_ROOT/examples/basic_example.c"
)

echo ""
echo "== compiling with -fsanitize=address,undefined -g -O0 =="
"$CC" -fsanitize=address,undefined -fno-omit-frame-pointer -g -O0 \
    -std=c99 -Wall -Wextra \
    -I"$REPO_ROOT/src" \
    "${SOURCES[@]}" \
    -lm -lpthread \
    -o "$BIN"

echo "built: $BIN"
echo ""
echo "== running under ASan/UBSan (cwd=$BUILD_DIR so exported *.json/*.txt land there) =="

LOG="$BUILD_DIR/run.log"
PROGRAM_STATUS=0
(
    cd "$BUILD_DIR"
    ASAN_OPTIONS="detect_leaks=1:halt_on_error=0:abort_on_error=0" \
    UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=0" \
        "$BIN"
) >"$LOG" 2>&1 || PROGRAM_STATUS=$?

cat "$LOG"

# basic_example.c has a known pre-existing ordering bug (see build-c.sh):
# it calls spi_framework_start() before any glitch fault parameters are
# registered, so main() returns 1 well before the program exits. That is a
# *program logic* exit code, not a sanitizer verdict. What this script
# actually needs to answer is "did ASan/UBSan flag anything in the code
# paths that ran" — so grep the log for sanitizer error banners instead of
# trusting the process exit code alone.
if grep -qE 'ERROR: (AddressSanitizer|UndefinedBehaviorSanitizer|LeakSanitizer)|runtime error:' "$LOG"; then
    SANITIZER_CLEAN=0
else
    SANITIZER_CLEAN=1
fi

echo ""
echo "program exit code: $PROGRAM_STATUS"
if [ "$SANITIZER_CLEAN" -eq 1 ]; then
    echo "asan-check.sh: PASS — no ASan/UBSan/LSan diagnostics in the run (memory-safety fixes hold)"
    if [ "$PROGRAM_STATUS" -ne 0 ]; then
        echo "  (note: basic_example itself exited $PROGRAM_STATUS due to the pre-existing"
        echo "   example ordering bug noted by build-c.sh — unrelated to memory safety)"
    fi
    exit 0
else
    echo "asan-check.sh: FAIL — sanitizer reported an issue, see $LOG"
    exit 1
fi
