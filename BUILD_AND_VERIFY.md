# SPIFault — Build & Verification

> *Quality gate for the HBO NL portfolio. This file answers:
> "Is this project buildable, runnable, and behaving as documented?"*

> *Last updated: 2026-05-14 — re-validated, no targeted fixes applied in this pass.*

## 1. Build

### 1.1 Primary build path

- **Platform**: Mixed — Python 3.10+ (FCDF schema, validator, simulator, CLI) and C99 (in-memory data model + reference simulator under `src/`).
- **Toolchain**:
  - Python: CPython 3.10 / 3.11 / 3.12 (matrix used in CI), `pip`, `pytest>=7.0`, `pyyaml>=6.0`.
  - C: `gcc` with `-Wall -Wextra -std=c99 -O2 -g`, `ar`, GNU `make`. Links against `-lm -lpthread`.
- **Command** (Python path — the documented primary path for running campaigns):
  ```bash
  pip install -r tests/requirements.txt
  python -m fcdf campaigns/voltage_glitch_basic.yaml
  ```
- **Command** (C path — builds the reference C libraries and the smoke-test example):
  ```bash
  make all
  ```
- **Output**:
  - Python run: `output/<campaign-id>.json` and `output/<campaign-id>.md` (e.g. `output/vg-001.json`, `output/vg-001.md`). CLI exits 0 if observed success rate is within the campaign's declared `[success_rate_min, success_rate_max]`, else 1.
  - C build: under `build/`:
    - `libspi_core.a`
    - `libinjection.a`
    - `libglitch.a`
    - `libspi_framework.a`
    - `basic_example` (linked executable from `examples/basic_example.c`).

### 1.2 Alternative build paths

- `make test` — builds the C libraries and example, then runs `build/basic_example` (in-process smoke test of the C side; no hardware).
- `make clean` — removes `build/` and `dist/`.
- `make dist` — packages source + build into `dist/spi_framework.tar.gz`.
- `make install` — copies static libs to `/usr/local/lib` and headers to `/usr/local/include/spi_framework/` (POSIX paths; not applicable on Windows without WSL/MSYS).
- `python -m fcdf <campaign.yaml> --out <dir>` — override report directory.
- `python -m fcdf <campaign.yaml> --backend simulator` — override the campaign-declared backend; currently only `simulator` is registered in `fcdf/runner.py::BACKENDS`.
- `python -m pytest tests/ -v` — run the Python test suite.

### 1.3 Dependencies

- **Required**:
  - Python `>=3.10` (CI matrix: 3.10, 3.11, 3.12).
  - `pyyaml>=6.0` (campaign YAML loader).
  - `pytest>=7.0` (test runner).
  - For C side: `gcc`, `make`, `ar`, POSIX-ish environment (the `Makefile` uses `mkdir -p`, `rm -rf`, `cp`, `tar` — on Windows use WSL, MSYS2/MinGW, or Git Bash).
- **Optional** (none required to run/validate the primary Python path; these are *documented integration points*, not implementations):
  - `chipwhisperer` Python package + ChipWhisperer Lite/Husky hardware — for the `chipwhisperer` backend (interface only — not implemented).
  - Raspberry Pi with `/dev/spidev*` + GPIO — for the `rpi_spidev` backend (interface only).
  - `sigrok` + logic-analyser captures — for the `sigrok` backend (interface only).
  - `jsonschema` — listed in the README's install line but **not** in `tests/requirements.txt`; ⚠️ inconsistency, see §3.1.

## 2. Expected behavior

### 2.1 What this project does

SPIFault is a **Fault Campaign Description Framework (FCDF)** for SPI-flash-based boot pipelines. It is observability/simulation infrastructure, not a fault-injection rig:

1. **Describes** a fault-injection campaign in a versioned YAML schema (`schema_version: 1`) with declarative sections: `campaign` (id/name/description), `target` (backend device, component enum, optional address), `fault` (model enum, intensity 0..1, optional bit_position), `glitch` (type enum + duration/voltage/clock/burst params), `trial` (repetitions, seed), and `expected_outcome` (classification enum, success-rate band).
2. **Validates** a YAML document with `fcdf/loader.py` (parsing + enum coercion) and `fcdf/schema.py::validate` (cross-field invariants — version, ranges, conditional required fields such as bit-flip requiring `bit_position`, voltage glitch requiring both voltage levels).
3. **Simulates** the campaign deterministically with `fcdf/simulator.py::SimulatorBackend` — for each repetition it samples an `OutcomeClass` from a fault-model baseline distribution, biased by `fault.intensity` toward the declared success class and (for voltage glitches) by a voltage-droop heuristic. Given the same `trial.seed` the simulator is reproducible.
4. **Reports** results to `output/<campaign-id>.json` (machine-readable report) and `output/<campaign-id>.md` (human-readable summary), recording per-class outcome counts, observed `success_rate`, the expected band, and a `within_expected` boolean. CLI exit code reflects `within_expected`.
5. **Pluggable backends**: a `Backend` interface contract is defined (a callable taking a `Campaign` and returning a `CampaignReport`); the `simulator` is the reference implementation, `chipwhisperer` / `rpi_spidev` / `sigrok` are documented but not implemented.
6. The C library under `src/` (`spi_core`, `injection_engine`, `glitch_engine`, `spi_framework`) provides the in-memory data model the Python simulator wraps; `examples/basic_example.c` is an end-to-end smoke test.

### 2.2 Acceptance criteria

- [ ] `pip install -r tests/requirements.txt` succeeds (resolves `pytest>=7.0`, `pyyaml>=6.0`).
- [ ] `python -m pytest tests/ -v` runs and **all tests pass** (test_schema.py — schema validation, test_loader.py — YAML round-trip, test_simulator.py — determinism + intensity/voltage heuristics + per-shipped-campaign smoke).
- [ ] `python -m fcdf campaigns/voltage_glitch_basic.yaml` runs without error, prints a line of the form `ran vg-001 on backend=simulator: success_rate=<float>, within_expected=<bool>`, and creates `output/vg-001.json` plus `output/vg-001.md`.
- [ ] Each of the five sample campaigns in `campaigns/` (`voltage_glitch_basic.yaml`, `single_bit_flip_memory.yaml`, `instruction_skip_branch.yaml`, `multi_glitch_burst.yaml`, `spi_boot_replay.yaml`) loads and runs (covered by `tests/test_simulator.py::test_each_sample_campaign_runs`).
- [ ] The simulator is deterministic given a fixed seed (covered by `tests/test_simulator.py::test_simulator_is_deterministic_given_seed`).
- [ ] An intentionally malformed YAML (e.g. missing `glitch.duration_ns`, `fault.intensity > 1.0`, voltage glitch without both voltages, single bit-flip without `bit_position`) is rejected by the loader with a `SchemaError` whose message is path-qualified.
- [ ] `make all` builds the four static libraries and `basic_example` under `build/` on a POSIX toolchain.
- [ ] `make test` runs `build/basic_example` and exits 0.
- [ ] GitHub Actions workflow `.github/workflows/python-tests.yml` green on the matrix (`ubuntu-latest`, Python 3.10/3.11/3.12).

### 2.3 Manual verification

```bash
# 1. Install Python deps
pip install -r tests/requirements.txt

# 2. Validate + simulate the shipped voltage-glitch campaign
python -m fcdf campaigns/voltage_glitch_basic.yaml --out output

# Expected stdout (success_rate is deterministic for seed=0xC0FFEE):
#   ran vg-001 on backend=simulator: success_rate=<≈0.7>, within_expected=True

# 3. Inspect generated artefacts
ls output/
#   vg-001.json   vg-001.md

cat output/vg-001.md
#   # Campaign report: `vg-001`
#   **Name**: Voltage glitch — basic Plundervolt-shape
#   ...
#   ## Outcome counts
#   - `benign`: ...
#   - `instruction_corruption`: ...
#   ## Success rate
#   - observed: 0.xxx
#   - expected: [0.100, 0.950]
#   - within expected: yes

# 4. Negative-path check: schema rejection of a bad campaign
python -c "from fcdf import load_campaign; load_campaign('campaigns/voltage_glitch_basic.yaml'); print('ok')"
# Expected: ok

# 5. Build and smoke-test the C side (POSIX shell: WSL / MSYS2 / Git Bash on Windows)
make all
ls build/
#   libspi_core.a  libinjection.a  libglitch.a  libspi_framework.a  basic_example
make test
# Expected: basic_example runs, exits 0, prints SPI transaction / glitch demo output
```

### 2.4 Automated verification

```bash
# Full Python test suite (15 tests across test_schema.py, test_loader.py, test_simulator.py)
python -m pytest tests/ -v

# CI mirror — `.github/workflows/python-tests.yml`:
#  - pip install -r tests/requirements.txt
#  - python -m pytest tests/ -v
#  - python -m fcdf campaigns/voltage_glitch_basic.yaml --out /tmp/fcdf-out
#  - asserts /tmp/fcdf-out/vg-001.json and /tmp/fcdf-out/vg-001.md exist

# C smoke test
make test
```

Test coverage map:

| File | Covers |
|---|---|
| `tests/test_schema.py` | Schema invariants: version, empty id, intensity range, bit-flip requires bit_position, voltage requires both voltages, clock requires both clocks, duration > 0, repetitions >= 1, success_rate_min <= success_rate_max. |
| `tests/test_loader.py` | YAML → Campaign loader correctness and round-trip via `dump_campaign`. |
| `tests/test_simulator.py` | Determinism on fixed seed, seed sensitivity, intensity monotonicity, voltage-droop monotonicity, outcome counts sum to repetitions, `within_expected` semantics, and a parametrised smoke run of each shipped campaign YAML. |

## 3. Alternatives & improvements

### 3.1 Known fragilities

- ~~⚠️ **Dependency drift between README and `tests/requirements.txt`** (README mentioned `jsonschema`, never imported).~~ **FIXED 2026-06-10** — README install comment is now `# PyYAML, pytest`; `fcdf/schema.py`'s hand-rolled dataclass validation is the only validator and needs no extra dep.
- ~~⚠️ **`Makefile` is POSIX-only** ... not documented in the README's build section.~~ **FIXED 2026-06-10** — the README "C library" section now notes the Makefile is POSIX-only and to build under WSL/MSYS2/Git Bash on Windows.
- ⚠️ **No `pyproject.toml` / `setup.py`**: `fcdf` is run as `python -m fcdf` and tests rely on `tests/conftest.py` inserting the repo root onto `sys.path`. Installation as a package (`pip install .`) is not supported. *(Still open — design choice.)*
- ~~⚠️ **Backends dict is hard-coded** ... No plugin-discovery mechanism (entry points / registry decorator).~~ **FIXED 2026-06-10** — backends now self-register via the `@register_backend` decorator (`fcdf/registry.py`); the runner resolves by name through `get_backend()` and never needs editing to add a backend. `SimulatorBackend` is the reference registration; `tests/test_registry.py` covers resolution, the no-name error, and name-conflict rejection.
- ~~⚠️ **CI smoke-test masks failure** ... runs the CLI smoke with `|| true` ...~~ **FIXED 2026-06-10** — the `|| true` was removed; a CLI crash now fails the workflow at the smoke step.
- ~~⚠️ **`build/` is checked into the repo as an empty directory** even though `.gitignore` excludes `build/`.~~ **FIXED 2026-06-10** — the stray empty `build/` directory was removed (it was never tracked by git, only a local artefact).
- ❌ **No real hardware backend implemented** — explicitly out of scope per README §"Backends", but worth noting that "ChipWhisperer / rpi_spidev / sigrok" rows in that table are aspirational.
- ⚠️ **Simulator's outcome distributions are heuristic, not empirically calibrated** — acknowledged in README §"Limitations" and in `fcdf/simulator.py` docstring. Real-hardware noise is not modelled.
- ⚠️ **EM and laser fault types** are present in the `GlitchType` enum but have no validation rules beyond the generic duration/burst checks, and no specific simulator bias — they degrade to a generic intensity-driven sampling. Documented as placeholders in README §"Limitations".

### 3.2 Fallbacks

- **No `make` available / Windows native shell**: the C library is optional for the primary deliverable. The Python path (validation + simulator + reports) is self-sufficient — `python -m fcdf campaigns/voltage_glitch_basic.yaml` covers the documented user story.
- **No `pyyaml` available**: `fcdf/loader.py` raises a clear `ImportError` with installation hint at import time, rather than failing deep in YAML parsing.
- **Unknown backend requested**: `runner.run_campaign` raises `ValueError` listing the available backend names rather than silently mis-routing.
- **PyYAML version conflict**: pin or upgrade via `pip install --upgrade pyyaml`.

### 3.3 Roadmap

- **Short-term (<=1 day)**:
  - Reconcile README install line with `tests/requirements.txt` (drop `jsonschema` from the README or add it to requirements).
  - Add a Windows note to the README C-library section (WSL/MSYS2/Git Bash required for `make`).
  - Remove the `|| true` from the CI CLI smoke step so a CLI crash fails the workflow at the right step.
  - Add a `pyproject.toml` so `pip install -e .` works and the `python -m fcdf` `sys.path` trick can be retired.
- **Medium-term (<=1 week)**:
  - Convert hand-rolled validation to a published JSON Schema (`schemas/fcdf-v1.json`) and validate via `jsonschema` so external tools can also lint campaigns.
  - Add a backend registry/entry-point mechanism so third parties can ship a backend without editing `runner.py`.
  - Calibrate the simulator's `_BASE_BIAS` distributions against published Plundervolt/CLKSCREW/ChipWhisperer data and cite sources in the source comments.
  - Add a CLI subcommand `python -m fcdf validate <file>` (validate-only, no run) for quick lint of contributed campaigns.
  - Add Windows-runner job to the CI matrix (or document the lack thereof).
- **Long-term**:
  - Implement at least one real hardware backend (most likely `chipwhisperer`, given the closest existing Python ecosystem) so the contract is exercised by more than the in-process simulator.
  - Model fault-noise distributions (per-trial outcome jitter) so the simulator can mimic the variance reported in real hardware studies.
  - Add EM and laser glitch-type-specific parameters + simulator bias.
  - Schema v2 with backwards-compatibility shim in the loader.

## 4. Sanity check

| Check | Status | Notes |
|-------|--------|-------|
| README documents build | OK | README §"Running a campaign", §"C library", §"Testing" give exact commands for the Python and C paths and for tests. |
| Build files present | OK | `Makefile` (C path) and `tests/requirements.txt` (Python deps) both present. No `pyproject.toml` — the Python package is run via `python -m fcdf` thanks to `fcdf/__main__.py`. |
| Build files internally consistent | OK | `Makefile` targets (`libspi_core.a`, `libinjection.a`, `libglitch.a`, `libspi_framework.a`, `basic_example`) match what `README` advertises. The README install comment now matches `tests/requirements.txt` (`PyYAML, pytest`) — the stale `jsonschema` mention was removed 2026-06-10. |
| Source structure matches README | OK | `src/spi_core.{c,h}`, `src/injection_engine.{c,h}`, `src/glitch_engine.{c,h}`, `src/spi_framework.{c,h}`, `fcdf/{schema,loader,simulator,cli,__main__,__init__,runner}.py`, `campaigns/*.yaml`, `examples/basic_example.c`, `config/framework_config.yaml`, `tests/`, `docs/` all exist as documented. (Minor: README lists `fcdf/cli.py`; the actual CLI lives in `fcdf/__main__.py` plus `fcdf/runner.py` — same effective surface, slightly different filename.) |
| Tests exist | OK | `tests/test_schema.py` (11 cases), `tests/test_loader.py`, `tests/test_simulator.py` (7 unit cases + 5 parametrised per-campaign smoke runs), `tests/conftest.py`, `tests/requirements.txt`. |
| Test invocation documented | OK | README §"Testing": `pip install -r tests/requirements.txt` then `python -m pytest tests/ -v`. CI mirror in `.github/workflows/python-tests.yml`. |
| Output artifacts named in build files | OK | `Makefile` names the four `.a` archives and `basic_example`. Python CLI (`fcdf/runner.py`) names `output/<campaign-id>.json` and `output/<campaign-id>.md` and the README documents this. ⚠️ Output directory is `output/` per `__main__.py` default; CI workflow uses `/tmp/fcdf-out` via `--out` — consistent because the flag is documented. |
