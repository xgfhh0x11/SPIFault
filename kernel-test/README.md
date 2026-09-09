# kernel-test/ — dynamic test infrastructure for SPIFault

## What this is, and what it is not

**SPIFault is a SIMULATION-ONLY framework.** Per the repo's own README
("Scope and posture"), the only implemented backend is an in-process,
deterministic, in-memory `simulator`. It does not open a SPI device, does
not apply a real voltage glitch, and does not modify firmware on any
storage medium. `chipwhisperer` / `rpi_spidev` / `sigrok` are named as
*documented integration points* in the backend table — none of them
exist as code.

There is **no kernel component in this repository at all**: no
`Kbuild`/`Kconfig`, no `module_init`/`module_exit`, no `.ko` target, no
`spidev` ioctls. The C code under `src/` is a userspace static-library
data model (`libspi_core.a`, `libinjection.a`, `libglitch.a`,
`libspi_framework.a`) linked into a userspace example binary
(`examples/basic_example.c`). It is named `kernel-test/` only to keep
directory naming consistent with sibling repos in this review batch —
treat that as a label, not a claim.

**A real hardware backend (ChipWhisperer over USB, or a Raspberry Pi
`spidev` backend) is future work**, not implemented here. Anyone wiring
one up should implement the `Backend` interface documented in
`fcdf/registry.py` / `README.md` ("Backends") and add the safety gates
called out in the repo review (dry-run default, explicit hardware-arm
flag) before it touches real hardware.

So "dynamic testing" in this directory means two things:

1. Running the FCDF **simulator campaigns end-to-end** (validate → run →
   JSON+Markdown report) — this is SPIFault's actual, working product.
2. **Building and exercising the C data-model library** (`src/`) as a
   userspace library, including under AddressSanitizer/UBSan, to confirm
   the memory-safety fixes in `injection_engine.c` (heap-overflow from a
   `uint16_t` length truncation, an off-by-one OOB read, an unbounded
   `strstr` over non-NUL-terminated data — see the repo review) hold.

Note also (from the review): the Python simulator does **not** call into
the C library — they are independent implementations of the same FCDF
semantics (`docs/architecture.md` says as much: "today: re-implements
semantics in Python; future: FFI"). Building the C side and running the
Python campaigns are two separate checks below, not one pipeline.

## Layout

```
kernel-test/
  README.md          this file
  build-c.sh          build src/ + examples/ via the repo Makefile
  run-campaign.sh     validate + run the shipped FCDF campaigns (Python simulator)
  asan-check.sh       build src/ + basic_example.c under ASan+UBSan and run it
  verify.sh           run pytest + sanity-check the campaign report outputs
  output/             (gitignored) campaign reports from run-campaign.sh
  build-asan/         (gitignored) ASan build artifacts
```

`build/`, `dist/`, `output/`, `*.o`/`*.a`/`.pytest_cache/` etc. are
already covered by the repo's root `.gitignore` (its patterns have no
leading `/`, so they apply recursively — `kernel-test/build/`,
`kernel-test/output/`, `kernel-test/build-asan/` are all ignored without
any extra `.gitignore` in this directory).

## Environment

- **Python side** (`run-campaign.sh`, `verify.sh`): works on Windows
  (Git Bash / native Python) or WSL2/Linux. Requires Python 3.10+,
  PyYAML, pytest (`tests/requirements.txt`; scripts auto-install if
  missing).
- **C side** (`build-c.sh`, `asan-check.sh`): requires a POSIX toolchain
  (gcc/clang, make, ar) — the Makefile and sources use `unistd.h`,
  `sys/time.h`, `usleep`, `gettimeofday`. This does **not** build under
  plain Windows Git Bash/PowerShell. Use **WSL2 Ubuntu-22.04**:

  ```bash
  wsl -d Ubuntu-22.04
  sudo apt-get update && sudo apt-get install -y build-essential clang
  ```

  If gcc is missing, both `build-c.sh` and `asan-check.sh` print that
  `apt-get` line and exit non-zero rather than failing opaquely.

## Running it

From `kernel-test/` (WSL2 Ubuntu-22.04 shell, repo checked out under
`/mnt/c/...` or a native Linux path):

```bash
cd kernel-test

# 1. Build the C data-model library + basic_example via the repo Makefile.
./build-c.sh

# 2. Run the shipped FCDF campaigns end-to-end through the Python simulator.
#    Validates each campaigns/*.yaml against fcdf/schema.py, runs it on the
#    "simulator" backend, writes output/<campaign-id>.{json,md}.
./run-campaign.sh
# or a single campaign:
./run-campaign.sh ../campaigns/voltage_glitch_basic.yaml

# 3. Rebuild under ASan+UBSan and run basic_example, to exercise the
#    injection_engine.c paths that had the memory-safety bugs.
./asan-check.sh

# 4. Run the pytest suite and sanity-check the campaign report outputs
#    produced by run-campaign.sh (well-formed JSON, required keys,
#    success_rate in [0,1], companion .md present).
./verify.sh
```

`run-campaign.sh` and `verify.sh` only need Python and can be run from
plain Windows Git Bash too; `build-c.sh` and `asan-check.sh` need WSL2 (or
another Linux/macOS shell with gcc/clang).

## What each script actually wires into

- **`build-c.sh`** → `make clean && make all && make test` (repo
  `Makefile` targets `all` and `test`; `test` runs `build/basic_example`).
- **`run-campaign.sh`** → `fcdf.loader.load_campaign()` (schema
  validation) then `python -m fcdf <campaign.yaml> --out <dir>`
  (`fcdf/__main__.py` → `fcdf.runner.run_campaign()`), against every
  file in `campaigns/*.yaml` by default (`voltage_glitch_basic.yaml`,
  `single_bit_flip_memory.yaml`, `instruction_skip_branch.yaml`,
  `multi_glitch_burst.yaml`, `spi_boot_replay.yaml`).
- **`asan-check.sh`** → compiles `src/spi_core.c`,
  `src/injection_engine.c`, `src/glitch_engine.c`, `src/spi_framework.c`,
  `examples/basic_example.c` directly (not via the Makefile, which has
  no sanitizer target) with `-fsanitize=address,undefined -g -O0`, then
  runs the resulting binary.
- **`verify.sh`** → `python -m pytest -q -p no:cacheprovider` (repo
  `tests/`) plus a JSON well-formedness check over
  `kernel-test/output/*.json`.

## Known limits (carried over from the repo review)

- No kernel module, no real `spidev`/ChipWhisperer backend — nothing
  here exercises real hardware or real-time kernel behavior. A real
  backend is future work; see "Backends" in the top-level `README.md`.
- C `execution_time_us` fields are computed from `time()` (second
  resolution), not a real microsecond clock — timing numbers in the
  C-side JSON export are not trustworthy at sub-second granularity.
- EM/laser/temperature/power_supply glitch types and several fault
  models collapse to generic intensity sampling in both the Python and C
  models — there is no type-specific physical model to validate against.
