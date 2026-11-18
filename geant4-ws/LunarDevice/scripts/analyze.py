#!/usr/bin/env python3
"""Summarize multi-worker CSV from LunarRecorder (Python stdlib only)."""
import argparse
import csv
import glob
import math
from collections import Counter
from pathlib import Path


def records(folder, stem):
    for path in sorted(glob.glob(str(folder / f"{stem}_r*_t*.csv"))):
        with open(path, newline="") as file:
            yield from csv.DictReader(file)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", default="results", type=Path)
    args = parser.parse_args()
    events = list(records(args.dir, "events"))
    if not events:
        raise SystemExit(f"No event CSV files under {args.dir}. Did the simulation run?")
    n = len(events)
    total_edep = sum(float(row["Si_edep_MeV"]) for row in events)
    n_hit = sum(float(row["Si_edep_MeV"]) > 0 for row in events)
    enter_al = sum(int(row["primary_entered_Al"]) for row in events)
    enter_cavity = sum(int(row["primary_entered_cavity"]) for row in events)
    reaction = sum(int(row["proton_inelastic_in_Al"]) for row in events)
    n_reaction_events = sum(int(row["proton_inelastic_in_Al"]) > 0 for row in events)
    neutrons = sum(int(row["neutrons_created_in_Al"]) for row in events)
    si_neutrons = sum(int(row["neutrons_entered_Si"]) for row in events)
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
    reaching = [float(row["primary_energy_at_cavity_MeV"]) for row in events
                if int(row["primary_entered_cavity"]) and float(row["primary_energy_at_cavity_MeV"]) >= 0]
    if reaching:
        mean_energy = sum(reaching) / len(reaching)
        print(f"Mean primary proton energy at cavity:     {mean_energy:.6f} MeV")
        print(f"Mean energy loss before cavity (100 MeV beam): {100 - mean_energy:.6f} MeV")
        print("  (Only among cavity-entering protons; 100 MeV hard-coded source assumption.)")
    neutron_energies = [float(row["energy_MeV"]) for row in records(args.dir, "neutrons")]
    if neutron_energies:
        print("Neutron source energies [MeV]: min={:.5g}, median={:.5g}, max={:.5g}".format(
            min(neutron_energies), sorted(neutron_energies)[len(neutron_energies)//2], max(neutron_energies)))


if __name__ == "__main__":
    main()
