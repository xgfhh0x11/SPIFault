"""FCDF v1 — schema definition and validation.

A campaign is a single declarative document describing one fault-injection
experiment: target, fault model, glitch parameters, trial parameters, and
the expected outcome. Documents are stored as YAML; this module provides
the Python representation, validation rules, and helpers.

Design notes:
- Dataclasses, not pydantic — keep the dependency surface minimal.
- Validation raises SchemaError with a path-qualified message so a user
  can locate the offending field.
- Enums are str-valued so YAML serialises them as strings.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Optional


SCHEMA_VERSION = 1


class SchemaError(ValueError):
    """Raised when a campaign document fails validation. The message starts
    with the dotted path of the offending field (e.g. 'glitch.duration_ns')."""


class FaultModel(str, Enum):
    BIT_FLIP_SINGLE = 'single_bit_flip'
    BIT_FLIP_MULTI = 'multi_bit_flip'
    BYTE_CORRUPTION = 'byte_corruption'
    INSTRUCTION_SKIP = 'instruction_skip'
    BRANCH_OVERRIDE = 'branch_override'
    MEMORY_CORRUPTION = 'memory_corruption'
    REGISTER_CORRUPTION = 'register_corruption'


class GlitchType(str, Enum):
    VOLTAGE = 'voltage'
    CLOCK = 'clock'
    ELECTROMAGNETIC = 'electromagnetic'
    LASER = 'laser'
    TEMPERATURE = 'temperature'
    POWER_SUPPLY = 'power_supply'


class Component(str, Enum):
    CPU_CORE = 'cpu_core'
    MEMORY = 'memory'
    CACHE = 'cache'
    REGISTERS = 'registers'
    BUS = 'bus'
    CLOCK = 'clock'
    POWER = 'power'
    SPI_BUS = 'spi_bus'


class OutcomeClass(str, Enum):
    BENIGN = 'benign'
    CRASH = 'crash'
    INSTRUCTION_CORRUPTION = 'instruction_corruption'
    DATA_CORRUPTION = 'data_corruption'
    CONTROL_FLOW_DIVERSION = 'control_flow_diversion'
    UNKNOWN = 'unknown'


@dataclass
class CampaignMeta:
    id: str
    name: str
    description: str = ''


@dataclass
class Target:
    device: str                         # backend name
    component: Component
    address: Optional[int] = None       # target memory/register address, if any


@dataclass
class Fault:
    model: FaultModel
    intensity: float = 1.0              # 0.0 .. 1.0
    bit_position: Optional[int] = None  # for bit-flip models


@dataclass
class Glitch:
    type: GlitchType
    duration_ns: int
    trigger_delay_ns: int = 0
    base_voltage_v: Optional[float] = None    # voltage glitches
    glitch_voltage_v: Optional[float] = None
    base_clock_hz: Optional[int] = None       # clock glitches
    glitch_clock_hz: Optional[int] = None
    burst_count: int = 1
    burst_interval_ns: int = 0


@dataclass
class Trial:
    repetitions: int = 1
    seed: int = 0


@dataclass
class ExpectedOutcome:
    classification: OutcomeClass
    success_rate_min: float = 0.0
    success_rate_max: float = 1.0


@dataclass
class Campaign:
    schema_version: int
    campaign: CampaignMeta
    target: Target
    fault: Fault
    glitch: Glitch
    trial: Trial
    expected_outcome: ExpectedOutcome


# ----- validation -----

def _err(path: str, msg: str) -> SchemaError:
    return SchemaError(f'{path}: {msg}')


def _check_range(path: str, value, lo, hi):
    if value < lo or value > hi:
        raise _err(path, f'value {value!r} outside [{lo}, {hi}]')


def validate(campaign: Campaign) -> None:
    """Raise SchemaError if any cross-field invariant is violated."""

    if campaign.schema_version != SCHEMA_VERSION:
        raise _err('schema_version',
                   f'unsupported version {campaign.schema_version} (expected {SCHEMA_VERSION})')

    if not campaign.campaign.id:
        raise _err('campaign.id', 'must be non-empty')
    if not campaign.campaign.name:
        raise _err('campaign.name', 'must be non-empty')

    if not campaign.target.device:
        raise _err('target.device', 'must be non-empty')

    _check_range('fault.intensity', campaign.fault.intensity, 0.0, 1.0)

    if campaign.fault.model in (FaultModel.BIT_FLIP_SINGLE,):
        if campaign.fault.bit_position is None:
            raise _err('fault.bit_position',
                       f'required when fault.model is {campaign.fault.model.value!r}')
        _check_range('fault.bit_position', campaign.fault.bit_position, 0, 63)

    if campaign.glitch.duration_ns <= 0:
        raise _err('glitch.duration_ns', f'must be > 0, got {campaign.glitch.duration_ns}')
    if campaign.glitch.trigger_delay_ns < 0:
        raise _err('glitch.trigger_delay_ns',
                   f'must be >= 0, got {campaign.glitch.trigger_delay_ns}')
    if campaign.glitch.burst_count < 1:
        raise _err('glitch.burst_count', f'must be >= 1, got {campaign.glitch.burst_count}')
    if campaign.glitch.burst_interval_ns < 0:
        raise _err('glitch.burst_interval_ns',
                   f'must be >= 0, got {campaign.glitch.burst_interval_ns}')

    if campaign.glitch.type is GlitchType.VOLTAGE:
        if campaign.glitch.base_voltage_v is None or campaign.glitch.glitch_voltage_v is None:
            raise _err('glitch',
                       'voltage glitches require base_voltage_v and glitch_voltage_v')
        _check_range('glitch.base_voltage_v', campaign.glitch.base_voltage_v, 0.0, 10.0)
        _check_range('glitch.glitch_voltage_v', campaign.glitch.glitch_voltage_v, 0.0, 10.0)

    if campaign.glitch.type is GlitchType.CLOCK:
        if campaign.glitch.base_clock_hz is None or campaign.glitch.glitch_clock_hz is None:
            raise _err('glitch',
                       'clock glitches require base_clock_hz and glitch_clock_hz')

    if campaign.trial.repetitions < 1:
        raise _err('trial.repetitions',
                   f'must be >= 1, got {campaign.trial.repetitions}')

    _check_range('expected_outcome.success_rate_min',
                 campaign.expected_outcome.success_rate_min, 0.0, 1.0)
    _check_range('expected_outcome.success_rate_max',
                 campaign.expected_outcome.success_rate_max, 0.0, 1.0)
    if campaign.expected_outcome.success_rate_min > campaign.expected_outcome.success_rate_max:
        raise _err('expected_outcome',
                   'success_rate_min must be <= success_rate_max')
