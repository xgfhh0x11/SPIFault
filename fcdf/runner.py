"""Top-level orchestrator: load a campaign, run a backend, write reports."""

from __future__ import annotations

import json
import os
from typing import Optional

from .loader import load_campaign
# Importing the simulator module runs its @register_backend decorator,
# which populates the registry with the reference 'simulator' backend.
from .simulator import CampaignReport  # noqa: F401  (also triggers registration)
from .registry import get_backend, available_backends
from .schema import Campaign


def run_campaign(path: str,
                 output_dir: str = 'output',
                 backend_name: Optional[str] = None) -> CampaignReport:
    """Run a campaign and write a JSON + Markdown report.

    Args:
        path: campaign YAML file.
        output_dir: where to write reports.
        backend_name: override the campaign's declared backend.

    Returns:
        The CampaignReport (already persisted under output_dir).
    """
    campaign = load_campaign(path)
    backend_key = backend_name or campaign.target.device
    backend_cls = get_backend(backend_key)
    if backend_cls is None:
        valid = ', '.join(available_backends()) or '(none)'
        raise ValueError(
            f"unknown backend {backend_key!r}; available: {valid}"
        )

    backend = backend_cls()
    report = backend.run(campaign)

    os.makedirs(output_dir, exist_ok=True)
    base = os.path.join(output_dir, campaign.campaign.id)

    with open(base + '.json', 'w', encoding='utf-8') as f:
        json.dump(report.to_dict(), f, indent=2)

    with open(base + '.md', 'w', encoding='utf-8') as f:
        _write_markdown(f, campaign, report)

    return report


def _write_markdown(f, campaign: Campaign, report: CampaignReport) -> None:
    f.write(f"# Campaign report: `{campaign.campaign.id}`\n\n")
    f.write(f"**Name**: {campaign.campaign.name}\n\n")
    if campaign.campaign.description:
        f.write(f"{campaign.campaign.description}\n\n")
    f.write(f"**Backend**: `{report.backend}`\n\n")
    f.write(f"**Target**: `{campaign.target.component.value}` "
            f"on device `{campaign.target.device}`\n\n")
    f.write(f"**Fault model**: `{campaign.fault.model.value}` "
            f"(intensity={campaign.fault.intensity})\n\n")
    f.write(f"**Glitch**: `{campaign.glitch.type.value}`, "
            f"duration={campaign.glitch.duration_ns} ns, "
            f"trigger_delay={campaign.glitch.trigger_delay_ns} ns\n\n")
    f.write(f"**Trials**: {campaign.trial.repetitions} "
            f"(seed=0x{campaign.trial.seed:X})\n\n")
    f.write("## Outcome counts\n\n")
    for k, v in sorted(report.outcome_counts.items()):
        f.write(f"- `{k}`: {v}\n")
    f.write("\n")
    f.write(f"## Success rate\n\n")
    f.write(f"- observed: **{report.success_rate:.3f}**\n")
    f.write(f"- expected: [{report.expected_min:.3f}, {report.expected_max:.3f}]\n")
    f.write(f"- within expected: **{'yes' if report.within_expected else 'no'}**\n")
