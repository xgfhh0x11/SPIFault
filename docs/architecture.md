# Architecture

The repository is split into two cooperating layers:

```
+---------------------------------------------+
|                fcdf/  (Python)              |
|  schema  validator  loader  simulator       |
|         runner   __main__ (CLI)             |
+----------------------+----------------------+
                       | uses (today: re-implements semantics in Python;
                       |       future: FFI into the C simulator core)
                       v
+---------------------------------------------+
|                 src/   (C)                  |
|   spi_core  injection_engine  glitch_engine |
|              spi_framework                  |
+---------------------------------------------+
```

**fcdf/** is the FCDF (Fault Campaign Description Format) layer. It owns
the schema and is the entry point users interact with:

```bash
python -m fcdf campaigns/voltage_glitch_basic.yaml
```

The CLI loads a YAML campaign, validates it against `fcdf/schema.py`,
runs the simulator backend in `fcdf/simulator.py`, and writes a JSON +
Markdown report to `output/<campaign-id>.{json,md}`.

**src/** is the C library: an in-memory data model with JSON export,
a synthetic SPI transaction log, an injection-rule engine, and a glitch
parameter / fault parameter model. The C library compiles to four static
archives (`libspi_core.a`, `libinjection.a`, `libglitch.a`,
`libspi_framework.a`) consumed by `examples/basic_example.c`. The C
implementations of `spi_core_write_device` / `spi_core_read_device` are
deliberate placeholders — the data model is hardware-backend-agnostic.

## FCDF schema (v1) — quick reference

Every campaign declares exactly these top-level keys:

- `schema_version`: integer, currently `1`.
- `campaign`: `{id, name, description}` — identity + free-text summary.
- `target`: where the fault is conceptually applied.
  - `device`: backend name (e.g. `simulator`).
  - `component`: one of `cpu_core, memory, cache, registers, bus, clock, power, spi_bus`.
  - `address`: optional target address as integer.
- `fault`: the fault model and its strength.
  - `model`: one of `single_bit_flip, multi_bit_flip, byte_corruption, instruction_skip, branch_override, memory_corruption, register_corruption`.
  - `intensity`: real number in `[0.0, 1.0]`.
  - `bit_position`: integer in `[0, 63]`, required for single-bit flips.
- `glitch`: the physical glitch profile that delivers the fault.
  - `type`: one of `voltage, clock, electromagnetic, laser, temperature, power_supply`.
  - `duration_ns`: > 0.
  - `trigger_delay_ns`: >= 0.
  - voltage glitches additionally require `base_voltage_v` and
    `glitch_voltage_v` in `[0, 10]`.
  - clock glitches additionally require `base_clock_hz` and
    `glitch_clock_hz`.
  - `burst_count`, `burst_interval_ns`: repeated-pulse options.
- `trial`: `{repetitions, seed}` — campaign reproducibility.
- `expected_outcome`: `{classification, success_rate_min, success_rate_max}`.
  - `classification`: one of `benign, crash, instruction_corruption, data_corruption, control_flow_diversion, unknown`.

See `fcdf/schema.py` for the authoritative definition.

## Adding a backend

A backend is any class that satisfies the duck-typed contract:

```python
class MyBackend:
    name = 'my_backend'

    def run(self, campaign: fcdf.Campaign) -> fcdf.CampaignReport:
        ...
```

Register it by adding an entry to `fcdf.runner.BACKENDS` keyed by
`MyBackend.name`. Hardware backends are responsible for translating the
`fault` / `glitch` fields into device-specific commands and for
producing outcome classifications honestly — the simulator's
probabilistic outcome model is *not* the contract; the campaign + report
schemas are.

Suggested hardware integrations:

- **ChipWhisperer** — the `chipwhisperer` Python package exposes a
  voltage / clock glitcher API that maps cleanly onto FCDF's `glitch`
  block.
- **Raspberry Pi `spidev`** — for SPI-bus replay and fault-pattern
  campaigns, `/dev/spidev*` plus a small GPIO trigger circuit are
  sufficient.
- **sigrok** — for replaying captured logic-analyzer traces back through
  the simulator's SPI core.

## Limitations

- The C library's hardware I/O entry points (`spi_core_write_device`,
  `spi_core_read_device`) are stubs. A real device-backed C path is out
  of scope for the current revision.
- The Python simulator's outcome distribution is a documented heuristic
  (see `fcdf/simulator.py`, `_BASE_BIAS`). Real hardware will not match
  it — that is intentional; the schema is the cross-tool contract.
- EM and laser glitch types are accepted by the schema but the simulator
  treats them as opaque (no specialised semantics yet).
