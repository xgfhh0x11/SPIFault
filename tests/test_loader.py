"""YAML round-trip: load → dump → load yields an equal campaign, and
every shipped sample campaign loads cleanly."""
import glob
import os

import pytest

from fcdf.loader import load_campaign, dump_campaign
from fcdf.schema import SchemaError


_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.dirname(_HERE)
_CAMPAIGNS_DIR = os.path.join(_ROOT, 'campaigns')


@pytest.mark.parametrize(
    'path',
    sorted(glob.glob(os.path.join(_CAMPAIGNS_DIR, '*.yaml'))),
    ids=lambda p: os.path.basename(p),
)
def test_sample_campaign_loads(path):
    c = load_campaign(path)
    assert c.campaign.id
    assert c.trial.repetitions >= 1


def test_round_trip(tmp_path):
    src = os.path.join(_CAMPAIGNS_DIR, 'voltage_glitch_basic.yaml')
    c1 = load_campaign(src)

    out = tmp_path / 'round_trip.yaml'
    dump_campaign(c1, str(out))
    c2 = load_campaign(str(out))

    assert c1.campaign.id == c2.campaign.id
    assert c1.glitch.duration_ns == c2.glitch.duration_ns
    assert c1.fault.model == c2.fault.model
    assert c1.expected_outcome.classification == c2.expected_outcome.classification


def test_missing_required_field_rejected(tmp_path):
    bad = tmp_path / 'bad.yaml'
    bad.write_text(
        "schema_version: 1\n"
        "campaign:\n"
        "  id: bad-1\n"
        "  name: missing target\n"
        # target / fault / glitch / expected_outcome omitted on purpose
    )
    with pytest.raises(SchemaError, match='target'):
        load_campaign(str(bad))


def test_unknown_enum_value_rejected(tmp_path):
    bad = tmp_path / 'bad_enum.yaml'
    bad.write_text(
        "schema_version: 1\n"
        "campaign: {id: x, name: x}\n"
        "target: {device: simulator, component: cpu_core}\n"
        "fault: {model: not_a_real_model}\n"
        "glitch:\n"
        "  type: voltage\n"
        "  duration_ns: 10\n"
        "  base_voltage_v: 1.2\n"
        "  glitch_voltage_v: 0.9\n"
        "trial: {repetitions: 1, seed: 0}\n"
        "expected_outcome: {classification: benign}\n"
    )
    with pytest.raises(SchemaError, match='fault.model'):
        load_campaign(str(bad))
