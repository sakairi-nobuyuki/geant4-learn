#!/usr/bin/env python3
"""Summarize multi-worker CSV from LunarRecorder (Python stdlib only)."""
import argparse
import csv
import math
import sys
from pathlib import Path


EVENT_FIELDS = {
    "Si_edep_MeV": float,
    "primary_entered_Al": int,
    "primary_entered_cavity": int,
    "proton_inelastic_in_Al": int,
    "neutrons_created_in_Al": int,
    "neutrons_entered_Si": int,
    "primary_energy_at_cavity_MeV": float,
}


def records(paths, fields):
    for path in paths:
        try:
            with path.open(newline="") as file:
                reader = csv.DictReader(file, strict=True)
                missing = fields.keys() - set(reader.fieldnames or [])
                if missing:
                    raise ValueError(f"missing columns: {', '.join(sorted(missing))}")
                for row in reader:
                    try:
                        if None in row or None in row.values():
                            raise ValueError("wrong number of columns")
                        values = {name: convert(row[name]) for name, convert in fields.items()}
                        if not all(math.isfinite(value) for value in values.values()):
                            raise ValueError("nonfinite value")
                    except ValueError as error:
                        raise ValueError(f"line {reader.line_num}: {error}") from error
                    yield values
        except (OSError, ValueError, csv.Error) as error:
            raise SystemExit(f"Cannot analyze {path}: {error}") from error


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", default="results", type=Path,
                        help="directory containing saved LunarDevice CSVs (default: results)")
    args = parser.parse_args()
    event_paths = sorted(args.dir.glob("events_r*_t*.csv"))
    if not event_paths:
        raise SystemExit(f"No event CSV files under {args.dir}.")
    events = []
    for path in event_paths:
        worker_events = list(records([path], EVENT_FIELDS))
        if not worker_events:
            raise SystemExit(f"Event CSV {path} contains no event rows.")
        events.extend(worker_events)
    n = len(events)
    total_edep = sum(row["Si_edep_MeV"] for row in events)
    n_hit = sum(row["Si_edep_MeV"] > 0 for row in events)
    enter_al = sum(row["primary_entered_Al"] for row in events)
    enter_cavity = sum(row["primary_entered_cavity"] for row in events)
    reaction = sum(row["proton_inelastic_in_Al"] for row in events)
    n_reaction_events = sum(row["proton_inelastic_in_Al"] > 0 for row in events)
    neutrons = sum(row["neutrons_created_in_Al"] for row in events)
    neutron_paths = sorted(args.dir.glob("neutrons_r*_t*.csv"))
    expected_neutron_names = {
        path.name.replace("events_", "neutrons_", 1) for path in event_paths
    }
    neutron_energies = [row["energy_MeV"] for row in records(neutron_paths, {"energy_MeV": float})]
    if ({path.name for path in neutron_paths} != expected_neutron_names
            or len(neutron_energies) != neutrons):
        print("Warning: neutron CSVs do not match the event files or neutron count. "
              "Skipping neutron energy statistics.", file=sys.stderr)
        neutron_energies = []
    si_neutrons = sum(row["neutrons_entered_Si"] for row in events)
    mass_kg = 0.0001165  # for 10x10x0.5 mm silicon, 2.33 g/cm^3
    dose_Gy = total_edep * 1.602176634e-13 / mass_kg
    print(f"Number of primary events:                  {n:,}")
    print(f"Primary protons entering Al:               {enter_al:,} ({enter_al/n:.4%} / primary)")
    print(f"Primary protons entering cavity:           {enter_cavity:,} ({enter_cavity/n:.4%} / primary)")
    print(f"Proton inelastic interactions in Al:       {reaction:,} ({reaction/n:.6f} / primary)")
    print(f"Events with proton inelastic reactions:    {n_reaction_events:,}")
    print(f"Neutrons generated inside Al:              {neutrons:,} ({neutrons/n:.6f} / primary)")
    print(f"Neutron entries into Si (crossings):       {si_neutrons:,}")
    print(f"Events with nonzero Si energy deposition:  {n_hit:,} ({n_hit/n:.4%})")
    print(f"Total Si energy deposition:                {total_edep:.6f} MeV")
    print(f"Mean Si energy deposition per primary:     {total_edep/n:.6f} MeV")
    print(f"Si absorbed dose for simulated run:        {dose_Gy*1e6:.6f} microGy")
    print("Note: proton transmission here is a CAVITY ENTRY tally, not exit to open space.")
    print("Note: neutron entry count may include repeat crossings by the same track.")
    print("Note: direct neutron-step edep omits energy deposited by neutron-born charged tracks.")
    print("Note: not a lunar dose rate. The source has no physical flux normalization.")
    reaching = [row["primary_energy_at_cavity_MeV"] for row in events
                if row["primary_entered_cavity"] and row["primary_energy_at_cavity_MeV"] >= 0]
    if reaching:
        mean_energy = sum(reaching) / len(reaching)
        print(f"Mean primary proton energy at cavity:     {mean_energy:.6f} MeV")
        print(f"Mean energy loss before cavity (100 MeV beam): {100 - mean_energy:.6f} MeV")
        print("  (Only among cavity-entering protons; 100 MeV hard-coded source assumption.)")
    if neutron_energies:
        print("Neutron source energies [MeV]: min={:.5g}, median={:.5g}, max={:.5g}".format(
            min(neutron_energies), sorted(neutron_energies)[len(neutron_energies)//2], max(neutron_energies)))


if __name__ == "__main__":
    main()
