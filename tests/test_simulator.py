"""Simulator backend semantics: determinism, intensity bias, voltage
heuristics."""
import os

import pytest

from fcdf.loader import load_campaign
from fcdf.simulator import SimulatorBackend
from fcdf.schema import (
    Campaign, CampaignMeta, Target, Fault, Glitch, Trial, ExpectedOutcome,
    Component, FaultModel, GlitchType, OutcomeClass, SCHEMA_VERSION,
)


_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.dirname(_HERE)


def _campaign(seed=42, intensity=0.5, repetitions=200,
              success=OutcomeClass.INSTRUCTION_CORRUPTION):
    return Campaign(
        schema_version=SCHEMA_VERSION,
        campaign=CampaignMeta(id='unit-1', name='unit'),
        target=Target(device='simulator', component=Component.CPU_CORE,
                      address=None),
        fault=Fault(model=FaultModel.INSTRUCTION_SKIP, intensity=intensity),
        glitch=Glitch(type=GlitchType.VOLTAGE, duration_ns=50,
                      base_voltage_v=1.2, glitch_voltage_v=0.85),
        trial=Trial(repetitions=repetitions, seed=seed),
        expected_outcome=ExpectedOutcome(
            classification=success,
            success_rate_min=0.0, success_rate_max=1.0),
    )


def test_simulator_is_deterministic_given_seed():
    backend = SimulatorBackend()
    a = backend.run(_campaign(seed=1234, repetitions=500))
    b = backend.run(_campaign(seed=1234, repetitions=500))
    assert a.outcome_counts == b.outcome_counts
    assert a.success_rate == b.success_rate


def test_simulator_seed_affects_outcome():
    backend = SimulatorBackend()
    a = backend.run(_campaign(seed=1, repetitions=500))
    b = backend.run(_campaign(seed=2, repetitions=500))
    # Different seeds should produce different draws (overwhelmingly likely
    # at 500 trials).
    assert a.outcome_counts != b.outcome_counts


def test_higher_intensity_increases_success_rate():
    backend = SimulatorBackend()
    low = backend.run(_campaign(seed=99, intensity=0.0, repetitions=2000))
    high = backend.run(_campaign(seed=99, intensity=1.0, repetitions=2000))
    assert high.success_rate > low.success_rate


def test_deeper_voltage_droop_increases_success_rate():
    """Voltage heuristic: a deeper glitch should bias more toward the
    declared success outcome."""
    backend = SimulatorBackend()
    base = _campaign(seed=7, intensity=0.5, repetitions=2000)
    base.glitch.base_voltage_v = 1.2
    base.glitch.glitch_voltage_v = 1.1     # shallow droop
    shallow = backend.run(base)

    base.glitch.glitch_voltage_v = 0.6     # deep droop
    deep = backend.run(base)
    assert deep.success_rate >= shallow.success_rate


def test_outcome_counts_sum_to_repetitions():
    backend = SimulatorBackend()
    report = backend.run(_campaign(seed=0, repetitions=137))
    assert sum(report.outcome_counts.values()) == 137


def test_within_expected_flag_reflects_bounds():
    backend = SimulatorBackend()
    c = _campaign(seed=0, intensity=1.0, repetitions=1000)
    # Force the expected band to be impossibly narrow:
    c.expected_outcome.success_rate_min = 0.99
    c.expected_outcome.success_rate_max = 1.0
    report = backend.run(c)
    if report.success_rate >= 0.99:
        assert report.within_expected
    else:
        assert not report.within_expected


@pytest.mark.parametrize('path', [
    os.path.join(_ROOT, 'campaigns', 'voltage_glitch_basic.yaml'),
    os.path.join(_ROOT, 'campaigns', 'single_bit_flip_memory.yaml'),
    os.path.join(_ROOT, 'campaigns', 'instruction_skip_branch.yaml'),
    os.path.join(_ROOT, 'campaigns', 'multi_glitch_burst.yaml'),
    os.path.join(_ROOT, 'campaigns', 'spi_boot_replay.yaml'),
])
def test_each_sample_campaign_runs(path):
    campaign = load_campaign(path)
    report = SimulatorBackend().run(campaign)
    assert sum(report.outcome_counts.values()) == campaign.trial.repetitions
