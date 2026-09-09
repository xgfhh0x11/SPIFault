"""Schema validation: catches malformed campaigns at the boundary."""
import pytest

from fcdf.schema import (
    Campaign, CampaignMeta, Target, Fault, Glitch, Trial, ExpectedOutcome,
    Component, FaultModel, GlitchType, OutcomeClass,
    SchemaError, validate, SCHEMA_VERSION,
)


def _good_campaign(**overrides) -> Campaign:
    """Build a minimal valid campaign; overrides replace fields."""
    base = Campaign(
        schema_version=SCHEMA_VERSION,
        campaign=CampaignMeta(id='t-001', name='unit-test campaign'),
        target=Target(device='simulator', component=Component.CPU_CORE,
                      address=0x4000),
        fault=Fault(model=FaultModel.INSTRUCTION_SKIP, intensity=0.5),
        glitch=Glitch(type=GlitchType.VOLTAGE,
                      duration_ns=50, trigger_delay_ns=1000,
                      base_voltage_v=1.2, glitch_voltage_v=0.85),
        trial=Trial(repetitions=10, seed=42),
        expected_outcome=ExpectedOutcome(
            classification=OutcomeClass.INSTRUCTION_CORRUPTION,
            success_rate_min=0.0, success_rate_max=1.0),
    )
    for k, v in overrides.items():
        setattr(base, k, v)
    return base


def test_minimal_valid_campaign_passes():
    validate(_good_campaign())  # must not raise


def test_unsupported_schema_version_rejected():
    c = _good_campaign(schema_version=99)
    with pytest.raises(SchemaError, match='schema_version'):
        validate(c)


def test_empty_id_rejected():
    c = _good_campaign()
    c.campaign.id = ''
    with pytest.raises(SchemaError, match='campaign.id'):
        validate(c)


def test_intensity_out_of_range_rejected():
    c = _good_campaign()
    c.fault.intensity = 1.5
    with pytest.raises(SchemaError, match='fault.intensity'):
        validate(c)


def test_bit_flip_without_bit_position_rejected():
    c = _good_campaign()
    c.fault.model = FaultModel.BIT_FLIP_SINGLE
    c.fault.bit_position = None
    with pytest.raises(SchemaError, match='fault.bit_position'):
        validate(c)


def test_bit_flip_with_bit_position_passes():
    c = _good_campaign()
    c.fault.model = FaultModel.BIT_FLIP_SINGLE
    c.fault.bit_position = 7
    validate(c)


def test_voltage_glitch_requires_both_voltages():
    c = _good_campaign()
    c.glitch.base_voltage_v = None
    with pytest.raises(SchemaError, match='voltage'):
        validate(c)


def test_clock_glitch_requires_both_clocks():
    c = _good_campaign()
    c.glitch.type = GlitchType.CLOCK
    c.glitch.base_voltage_v = None
    c.glitch.glitch_voltage_v = None
    c.glitch.base_clock_hz = None
    c.glitch.glitch_clock_hz = None
    with pytest.raises(SchemaError, match='clock'):
        validate(c)


def test_zero_duration_glitch_rejected():
    c = _good_campaign()
    c.glitch.duration_ns = 0
    with pytest.raises(SchemaError, match='duration_ns'):
        validate(c)


def test_zero_repetitions_rejected():
    c = _good_campaign()
    c.trial.repetitions = 0
    with pytest.raises(SchemaError, match='trial.repetitions'):
        validate(c)


def test_success_rate_min_greater_than_max_rejected():
    c = _good_campaign()
    c.expected_outcome.success_rate_min = 0.8
    c.expected_outcome.success_rate_max = 0.2
    with pytest.raises(SchemaError, match='success_rate'):
        validate(c)
