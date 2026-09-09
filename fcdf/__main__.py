"""CLI entry point: `python -m fcdf <campaign.yaml>`"""

import argparse
import sys

from .runner import run_campaign


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog='python -m fcdf',
        description='Run a fault-injection campaign described in FCDF YAML.',
    )
    parser.add_argument('campaign', help='Path to the campaign YAML file.')
    parser.add_argument('--out', default='output',
                        help='Output directory for reports (default: output)')
    parser.add_argument('--backend', default=None,
                        help='Override the campaign-declared backend.')
    args = parser.parse_args(argv)

    report = run_campaign(args.campaign, output_dir=args.out,
                          backend_name=args.backend)
    print(f"ran {report.campaign_id} on backend={report.backend}: "
          f"success_rate={report.success_rate:.3f}, "
          f"within_expected={report.within_expected}")
    return 0 if report.within_expected else 1


if __name__ == '__main__':  # pragma: no cover
    sys.exit(main())
