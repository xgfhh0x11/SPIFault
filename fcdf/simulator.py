"""Reference simulator backend.

This backend is deterministic given the campaign's `trial.seed`. It does
not drive any hardware; it samples an outcome class per repetition from a
distribution derived from the fault model, fault intensity, and glitch
parameters.

The outcome distribution model is intentionally simple and documented in
the source. A real hardware backend is free to disagree — the schema and
report format are the contract, not these probabilities.
"""

from __future__ import annotations

import random
from dataclasses import dataclass, field
from typing import Dict, List

from .registry import register_backend

from .schema import (
    Campaign,
    FaultModel, GlitchType, OutcomeClass,
)


@dataclass
class TrialResult:
    repetition_index: int
    outcome: OutcomeClass
    notes: str = ''


@dataclass
class CampaignReport:
    campaign_id: str
    backend: str
    trials: List[TrialResult] = field(default_factory=list)
    outcome_counts: Dict[str, int] = field(default_factory=dict)
    success_rate: float = 0.0
    expected_min: float = 0.0
    expected_max: float = 1.0
    within_expected: bool = False

    def to_dict(self) -> dict:
        return {
            'campaign_id': self.campaign_id,
            'backend': self.backend,
            'trial_count': len(self.trials),
            'outcome_counts': self.outcome_counts,
            'success_rate': self.success_rate,
            'expected_min': self.expected_min,
            'expected_max': self.expected_max,
            'within_expected': self.within_expected,
        }


# Mapping from fault model + intensity to a baseline distribution of
# possible outcome classes. The "success" outcome is whatever the
# campaign declares in expected_outcome.classification.
_BASE_BIAS: Dict[FaultModel, Dict[OutcomeClass, float]] = {
    FaultModel.BIT_FLIP_SINGLE: {
        OutcomeClass.BENIGN: 0.50,
        OutcomeClass.DATA_CORRUPTION: 0.35,
        OutcomeClass.CRASH: 0.10,
        OutcomeClass.UNKNOWN: 0.05,
    },
    FaultModel.BIT_FLIP_MULTI: {
        OutcomeClass.BENIGN: 0.20,
        OutcomeClass.DATA_CORRUPTION: 0.55,
        OutcomeClass.CRASH: 0.20,
        OutcomeClass.UNKNOWN: 0.05,
    },
    FaultModel.BYTE_CORRUPTION: {
        OutcomeClass.BENIGN: 0.30,
        OutcomeClass.DATA_CORRUPTION: 0.50,
        OutcomeClass.CRASH: 0.15,
        OutcomeClass.UNKNOWN: 0.05,
    },
    FaultModel.INSTRUCTION_SKIP: {
        OutcomeClass.BENIGN: 0.30,
        OutcomeClass.INSTRUCTION_CORRUPTION: 0.45,
        OutcomeClass.CONTROL_FLOW_DIVERSION: 0.15,
        OutcomeClass.CRASH: 0.10,
    },
    FaultModel.BRANCH_OVERRIDE: {
        OutcomeClass.BENIGN: 0.25,
        OutcomeClass.CONTROL_FLOW_DIVERSION: 0.55,
        OutcomeClass.CRASH: 0.15,
        OutcomeClass.UNKNOWN: 0.05,
    },
    FaultModel.MEMORY_CORRUPTION: {
        OutcomeClass.BENIGN: 0.25,
        OutcomeClass.DATA_CORRUPTION: 0.50,
        OutcomeClass.CRASH: 0.20,
        OutcomeClass.UNKNOWN: 0.05,
    },
    FaultModel.REGISTER_CORRUPTION: {
        OutcomeClass.BENIGN: 0.30,
        OutcomeClass.DATA_CORRUPTION: 0.30,
        OutcomeClass.CONTROL_FLOW_DIVERSION: 0.25,
        OutcomeClass.CRASH: 0.15,
    },
}


def _apply_intensity(dist: Dict[OutcomeClass, float],
                     success_class: OutcomeClass,
                     intensity: float) -> Dict[OutcomeClass, float]:
    """Bend the baseline distribution toward `success_class` by `intensity`.
    intensity=0 leaves the baseline alone; intensity=1 puts almost all mass
    on the success class. Result is renormalised."""
    intensity = max(0.0, min(1.0, intensity))
    bent = {}
    benign_mass = dist.get(OutcomeClass.BENIGN, 0.0)
    pulled = benign_mass * intensity
    for k, v in dist.items():
        if k is OutcomeClass.BENIGN:
            bent[k] = v - pulled
        else:
            bent[k] = v
    bent[success_class] = bent.get(success_class, 0.0) + pulled

    # Renormalise to handle floating-point drift.
    total = sum(bent.values()) or 1.0
    return {k: v / total for k, v in bent.items()}


def _voltage_factor(g) -> float:
    """Heuristic: deeper voltage droops increase fault probability.
    Returns a multiplier in [0.5, 1.5]."""
    if g.base_voltage_v is None or g.glitch_voltage_v is None:
        return 1.0
    if g.base_voltage_v <= 0:
        return 1.0
    droop = max(0.0, (g.base_voltage_v - g.glitch_voltage_v) / g.base_voltage_v)
    return 1.0 + min(0.5, droop)


def _bias_outcome(dist: Dict[OutcomeClass, float],
                  success_class: OutcomeClass,
                  multiplier: float) -> Dict[OutcomeClass, float]:
    bent = dict(dist)
    bent[success_class] = bent.get(success_class, 0.0) * multiplier
    total = sum(bent.values()) or 1.0
    return {k: v / total for k, v in bent.items()}


def _sample(rng: random.Random, dist: Dict[OutcomeClass, float]) -> OutcomeClass:
    classes = list(dist.keys())
    weights = [dist[c] for c in classes]
    return rng.choices(classes, weights=weights, k=1)[0]


@register_backend
class SimulatorBackend:
    """In-process deterministic simulator. The contract for any backend is
    that, given a Campaign, it returns a CampaignReport."""

    name = 'simulator'

    def run(self, campaign: Campaign) -> CampaignReport:
        rng = random.Random(campaign.trial.seed)
        baseline = _BASE_BIAS[campaign.fault.model]
        success_class = campaign.expected_outcome.classification

        dist = _apply_intensity(baseline, success_class, campaign.fault.intensity)
        if campaign.glitch.type is GlitchType.VOLTAGE:
            dist = _bias_outcome(dist, success_class, _voltage_factor(campaign.glitch))

        results: List[TrialResult] = []
        counts: Dict[str, int] = {}
        success_hits = 0

        for i in range(campaign.trial.repetitions):
            outcome = _sample(rng, dist)
            results.append(TrialResult(repetition_index=i, outcome=outcome))
            counts[outcome.value] = counts.get(outcome.value, 0) + 1
            if outcome is success_class:
                success_hits += 1

        rate = success_hits / max(1, campaign.trial.repetitions)
        within = (campaign.expected_outcome.success_rate_min
                  <= rate
                  <= campaign.expected_outcome.success_rate_max)

        return CampaignReport(
            campaign_id=campaign.campaign.id,
            backend=self.name,
            trials=results,
            outcome_counts=counts,
            success_rate=rate,
            expected_min=campaign.expected_outcome.success_rate_min,
            expected_max=campaign.expected_outcome.success_rate_max,
            within_expected=within,
        )
