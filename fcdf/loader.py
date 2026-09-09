"""YAML <-> Campaign serialisation."""

from __future__ import annotations

from typing import Any, Mapping

try:
    import yaml  # PyYAML
except ImportError as e:  # pragma: no cover
    raise ImportError(
        'PyYAML is required for FCDF loader; install with `pip install pyyaml`'
    ) from e

from .schema import (
    SCHEMA_VERSION,
    Campaign, CampaignMeta, Target, Fault, Glitch, Trial, ExpectedOutcome,
    Component, FaultModel, GlitchType, OutcomeClass,
    SchemaError, validate,
)


def _require(d: Mapping[str, Any], key: str, path: str) -> Any:
    if key not in d:
        raise SchemaError(f'{path}: missing required field {key!r}')
    return d[key]


def _enum(enum_cls, value, path: str):
    try:
        return enum_cls(value)
    except ValueError as e:
        valid = ', '.join(m.value for m in enum_cls)
        raise SchemaError(f'{path}: unknown value {value!r}; valid: {valid}') from e


def _from_dict(doc: Mapping[str, Any]) -> Campaign:
    schema_version = _require(doc, 'schema_version', '')
    if not isinstance(schema_version, int):
        raise SchemaError(f"schema_version: expected int, got {type(schema_version).__name__}")

    c = _require(doc, 'campaign', '')
    meta = CampaignMeta(
        id=_require(c, 'id', 'campaign'),
        name=_require(c, 'name', 'campaign'),
        description=c.get('description', ''),
    )

    t = _require(doc, 'target', '')
    target = Target(
        device=_require(t, 'device', 'target'),
        component=_enum(Component, _require(t, 'component', 'target'), 'target.component'),
        address=t.get('address'),
    )

    f = _require(doc, 'fault', '')
    fault = Fault(
        model=_enum(FaultModel, _require(f, 'model', 'fault'), 'fault.model'),
        intensity=float(f.get('intensity', 1.0)),
        bit_position=f.get('bit_position'),
    )

    g = _require(doc, 'glitch', '')
    glitch = Glitch(
        type=_enum(GlitchType, _require(g, 'type', 'glitch'), 'glitch.type'),
        duration_ns=int(_require(g, 'duration_ns', 'glitch')),
        trigger_delay_ns=int(g.get('trigger_delay_ns', 0)),
        base_voltage_v=g.get('base_voltage_v'),
        glitch_voltage_v=g.get('glitch_voltage_v'),
        base_clock_hz=g.get('base_clock_hz'),
        glitch_clock_hz=g.get('glitch_clock_hz'),
        burst_count=int(g.get('burst_count', 1)),
        burst_interval_ns=int(g.get('burst_interval_ns', 0)),
    )

    tr = doc.get('trial', {}) or {}
    trial = Trial(
        repetitions=int(tr.get('repetitions', 1)),
        seed=int(tr.get('seed', 0)),
    )

    eo = _require(doc, 'expected_outcome', '')
    expected = ExpectedOutcome(
        classification=_enum(OutcomeClass,
                             _require(eo, 'classification', 'expected_outcome'),
                             'expected_outcome.classification'),
        success_rate_min=float(eo.get('success_rate_min', 0.0)),
        success_rate_max=float(eo.get('success_rate_max', 1.0)),
    )

    campaign = Campaign(
        schema_version=schema_version,
        campaign=meta,
        target=target,
        fault=fault,
        glitch=glitch,
        trial=trial,
        expected_outcome=expected,
    )
    validate(campaign)
    return campaign


def load_campaign(path: str) -> Campaign:
    """Load a campaign from a YAML file and validate it."""
    with open(path, 'r', encoding='utf-8') as f:
        doc = yaml.safe_load(f)
    if not isinstance(doc, Mapping):
        raise SchemaError('root: expected a YAML mapping')
    return _from_dict(doc)


def _to_dict(c: Campaign) -> dict:
    return {
        'schema_version': c.schema_version,
        'campaign': {
            'id': c.campaign.id,
            'name': c.campaign.name,
            'description': c.campaign.description,
        },
        'target': {
            'device': c.target.device,
            'component': c.target.component.value,
            'address': c.target.address,
        },
        'fault': {
            'model': c.fault.model.value,
            'intensity': c.fault.intensity,
            'bit_position': c.fault.bit_position,
        },
        'glitch': {
            'type': c.glitch.type.value,
            'duration_ns': c.glitch.duration_ns,
            'trigger_delay_ns': c.glitch.trigger_delay_ns,
            'base_voltage_v': c.glitch.base_voltage_v,
            'glitch_voltage_v': c.glitch.glitch_voltage_v,
            'base_clock_hz': c.glitch.base_clock_hz,
            'glitch_clock_hz': c.glitch.glitch_clock_hz,
            'burst_count': c.glitch.burst_count,
            'burst_interval_ns': c.glitch.burst_interval_ns,
        },
        'trial': {
            'repetitions': c.trial.repetitions,
            'seed': c.trial.seed,
        },
        'expected_outcome': {
            'classification': c.expected_outcome.classification.value,
            'success_rate_min': c.expected_outcome.success_rate_min,
            'success_rate_max': c.expected_outcome.success_rate_max,
        },
    }


def dump_campaign(campaign: Campaign, path: str) -> None:
    """Serialise a campaign to YAML."""
    with open(path, 'w', encoding='utf-8') as f:
        yaml.safe_dump(_to_dict(campaign), f, sort_keys=False, default_flow_style=False)
