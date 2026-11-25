#!/usr/bin/env python3
"""Compare stock enemy records with the measured macOS reference views at load."""

import argparse
import csv
import hashlib
from pathlib import Path
import subprocess
import sys

REFERENCE = Path(__file__).resolve().parents[1] / "docs/reference/exe/enemy-visibility.csv"


def compare(probe, lab, level):
    digest = hashlib.sha256(lab.read_bytes()).hexdigest()
    if digest != "acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c":
        raise ValueError(f"GAME.LAB differs from the stock reference (SHA-256 {digest})")
    differences = 0
    checked = 0
    with REFERENCE.open(encoding="ascii", newline="") as reference:
        for case in csv.DictReader(reference):
            if level is not None and int(case["level"]) != level:
                continue
            pose = [case[key] for key in ("level", "cam_sector", "cam_x", "cam_y", "cam_angle")]
            run = subprocess.run([probe, lab, *pose], capture_output=True, text=True)
            if run.returncode:
                raise RuntimeError(f"Level {case['level']}, record {case['record']}: {run.stderr.strip()}")
            selected = [r for r in csv.DictReader(run.stdout.splitlines())
                        if r["record"] == case["record"] and r["tick"] == "0"]
            if len(selected) != 1:
                raise ValueError(f"Expected one record {case['record']} in level {case['level']}")
            row = selected[0]
            checked += 1
            for key in ("instantiated", "result", "pixels"):
                if row[key] != case[key]:
                    differences += 1
                    print(f"Level {row['level']}, record {row['record']}, {key}: "
                          f"expected {case[key]}, observed {row[key]}")
    print(f"{checked} records checked, {differences} field differences")
    return int(differences != 0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("probe", type=Path)
    parser.add_argument("lab", type=Path)
    parser.add_argument("--level", type=int, choices=range(1, 31))
    args = parser.parse_args()
    try:
        sys.exit(compare(args.probe.resolve(), args.lab.resolve(), args.level))
    except (OSError, RuntimeError, ValueError) as error:
        parser.exit(1, f"Comparison failed: {error}\n")
