"""FCDF — Fault Campaign Description Format.

Public surface re-exports the data classes and the top-level operations:

    from fcdf import Campaign, load_campaign, validate, run_campaign

Schema version 1.
"""

from .schema import (
    SCHEMA_VERSION,
    Campaign,
    CampaignMeta,
    Target,
    Fault,
    Glitch,
    Trial,
    ExpectedOutcome,
    FaultModel,
    GlitchType,
    Component,
    OutcomeClass,
    SchemaError,
)
from .loader import load_campaign, dump_campaign
from .simulator import SimulatorBackend, TrialResult, CampaignReport
from .registry import register_backend, get_backend, available_backends
from .runner import run_campaign

__all__ = [
    'SCHEMA_VERSION',
    'Campaign', 'CampaignMeta', 'Target', 'Fault', 'Glitch',
    'Trial', 'ExpectedOutcome',
    'FaultModel', 'GlitchType', 'Component', 'OutcomeClass',
    'SchemaError',
    'load_campaign', 'dump_campaign',
    'SimulatorBackend', 'TrialResult', 'CampaignReport',
    'register_backend', 'get_backend', 'available_backends',
    'run_campaign',
]
