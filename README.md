# SPIFault — Fault Campaign Description Framework for SPI Flash

An open framework for **describing**, **simulating**, and **reporting**
fault-injection campaigns against SPI flash–based boot pipelines. Campaigns
are declared in a YAML schema (FCDF — Fault Campaign Description Format)
and executed against pluggable backends. A reference *simulator* backend
ships with the repo; *hardware* backends (ChipWhisperer, Raspberry Pi
`spidev`, sigrok) are documented integration points.

The goal is not to provide a fault-injection rig — those exist
(ChipWhisperer, Riscure Inspector, custom FPGA boards). The goal is to
provide the **specification, validator, and reproducible report tooling**
that the academic FI literature consistently lacks.

---

## Motivation

Fault-injection papers each invent their own campaign description format.
A glitch in Plundervolt is described in English prose, ChipWhisperer
demos as Python snippets, VoltJockey in Verilog timing tables, and so on.
There is no shared way to say:

> *"For 100 trials, apply a 50 ns voltage glitch at base 1.20 V → 0.85 V
>  with trigger offset 12 µs, targeting a memory address in the simulated
>  SPI read pipeline, model the fault as a single-bit flip at bit 3."*

…in a way that another lab can ingest and re-run.

SPIFault proposes one. The format is a YAML schema with a Python
validator. Campaigns can be archived, diff'd, hashed, and shared. The
reference simulator backend gives the schema concrete semantics; hardware
backends, when implemented, must respect the same semantics.

## Repository layout

```
src/                 C library: in-memory data model + reference simulator
  spi_core.{c,h}        SPI transaction log + JSON export
  injection_engine.{c,h} Payload + rule model
  glitch_engine.{c,h}   Voltage / fault parameter model
  spi_framework.{c,h}   Top-level wiring

examples/
  basic_example.c       End-to-end smoke test (in-process simulator)

config/
  framework_config.yaml Aspirational config sketch — not read by any code
                         path in fcdf/ or src/ (see "Limitations")

fcdf/                 Python: FCDF schema, validator, loader, runner
  schema.py             Campaign dataclasses + JSON-schema-ish validation
  loader.py             YAML → Campaign object
  simulator.py          Reference simulator backend
  registry.py            Backend registration (@register_backend)
  runner.py              Resolves a backend and runs a campaign
  __main__.py            python -m fcdf <campaign.yaml>

campaigns/            Sample campaigns (reproducible, version-tagged)
  voltage_glitch_basic.yaml
  single_bit_flip_memory.yaml
  instruction_skip_branch.yaml
  multi_glitch_burst.yaml
  spi_boot_replay.yaml

tests/                Python pytest suite

docs/
  architecture.md
```

## Scope and posture

This is **observability and simulation infrastructure**. The default backend
runs in process, in memory, and writes nothing to any device. It explicitly:

- does **not** drive real SPI chips
- does **not** apply real voltage glitches
- does **not** modify firmware on any storage medium

What it does do:

- read a campaign in FCDF YAML, validate it against the schema
- run a reproducible simulated trial — fault model semantics produce
  expected effect labels (no real hardware effect)
- write a structured report (JSON + Markdown) suitable for archival

Real hardware execution requires implementing a backend (see below) and
operating it on hardware that is your own, in a setting where you have
authorisation.

## FCDF — one campaign

```yaml
schema_version: 1

campaign:
  id: vg-001
  name: "Voltage glitch — basic Plundervolt-shape"
  description: |
    Single voltage-glitch trial against a simulated MSR write at the
    addressed location. Mirrors the timing envelope from Murdock et
    al. (Plundervolt, S&P 2020).

target:
  device: simulator       # backend name
  component: cpu_core
  address: 0xFFFFFFFF80000000

fault:
  model: instruction_skip
  intensity: 0.7
  bit_position: null

glitch:
  type: voltage
  base_voltage_v: 1.20
  glitch_voltage_v: 0.85
  duration_ns: 50
  trigger_delay_ns: 12000

trial:
  repetitions: 100
  seed: 0xC0FFEE

expected_outcome:
  classification: instruction_corruption
  success_rate_min: 0.0
  success_rate_max: 1.0
```

A complete schema reference lives in `fcdf/schema.py`. Sample campaigns
live in `campaigns/`.

## Running a campaign

```bash
pip install -r tests/requirements.txt    # PyYAML, pytest
python -m fcdf campaigns/voltage_glitch_basic.yaml
```

This validates the campaign against the schema, executes it against the
simulator backend, and writes `output/<campaign-id>.json` plus a
Markdown summary.

## Backends

The framework defines a `Backend` interface. The simulator is the
reference implementation; the other backends below are *documented
integration points*, not implementations:

| Backend | Status | Notes |
|---|---|---|
| `simulator` | implemented | in-process, deterministic given seed |
| `chipwhisperer` | interface only | requires ChipWhisperer Lite/Husky + `chipwhisperer` Python package |
| `rpi_spidev` | interface only | Raspberry Pi GPIO + `/dev/spidev*` |
| `sigrok` | interface only | logic-analyzer capture replay |

Contributions of real hardware backends are welcome — the simulator
defines the contract.

## C library

The C library under `src/` is a standalone in-memory data model (SPI
transaction log, injection-rule engine, glitch/fault parameter model)
with its own JSON export and `examples/basic_example.c` smoke test. It
is **not** currently wired to the Python simulator — `fcdf/simulator.py`
re-implements its own semantics directly in Python, and there is no
FFI/ctypes/cffi binding between the two layers today. See
`docs/architecture.md` for the current split and the planned direction.

Build:

```bash
make all          # libspi_core.a, libinjection.a, libglitch.a, libspi_framework.a
make test         # builds and runs examples/basic_example.c
make clean
```

> The `Makefile` is POSIX-only (it uses `mkdir -p`, `rm -rf`, `cp`,
> `tar`, and a `/usr/local` install prefix). On native Windows, build it
> under WSL, MSYS2, or Git Bash. The Python framework (`fcdf/`) has no
> such constraint and runs anywhere.

## Testing

```bash
pip install -r tests/requirements.txt
python -m pytest tests/ -v
```

Continuous integration runs the Python suite on every push under
`.github/workflows/python-tests.yml`.

## Limitations

- The simulator's fault models are deterministic given a seed; real hardware
  fault behaviour is noisy and the simulator does not currently model that
  noise distribution.
- Schema version 1 covers voltage / clock / temperature glitches and
  bit-flip / byte-corruption / instruction-skip / branch-override fault
  models. EM and laser fault injection are placeholders.
- No real hardware backend is implemented in this repository.
- `config/framework_config.yaml` is a planned/aspirational configuration
  sketch (CSV/HTML export, template directory, boot-phase labels, logic
  analyzer channels). It is not read by `fcdf/` or `src/` today; the CLI
  only ever writes `<id>.json` + `<id>.md`.

## References

- K. Murdock et al., *"Plundervolt: Software-based Fault Injection Attacks
  against Intel SGX,"* IEEE S&P, 2020.
- C. O'Flynn, *"ChipWhisperer: An Open-Source Platform for Hardware
  Embedded Security Research,"* COSADE, 2014.
- A. Tang, S. Sethumadhavan, S. Stolfo, *"CLKSCREW: Exposing the
  Perils of Security-Oblivious Energy Management,"* USENIX Security, 2017.
- A. Barenghi et al., *"Fault Injection Attacks on Cryptographic Devices:
  Theory, Practice, and Countermeasures,"* Proc. IEEE, 2012.

## License

MIT — see `LICENSE`.

## Status

Research / non-production. The framework is intended as infrastructure
for reproducible fault-injection research and as a teaching artefact;
real hardware experiments require additional discipline beyond what this
repository provides.
