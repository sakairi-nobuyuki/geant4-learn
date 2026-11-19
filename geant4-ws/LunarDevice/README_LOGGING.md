# LunarDevice particle-interaction logging

This overlay extends the **existing** Geant4 B1-derived `LunarDevice` source. It does not replace your customized detector geometry, source, visualization, Dockerfile, or the original B1 dose calculation.

## What is recorded

- `events_r<run>_t<worker>.csv`: one row per primary event, energy deposition in silicon, proton boundary crossings, nuclear reaction and neutron counts.
- `reactions_r<run>_t<worker>.csv`: `protonInelastic` steps **inside AlCase** (primary and secondary proton tracks).
- `neutrons_r<run>_t<worker>.csv`: neutrons **born inside AlCase**, with source process, position, direction and initial kinetic energy.
- `crossings_r<run>_t<worker>.csv`: selected geometry-boundary crossings: primary proton entering Al, primary proton entering cavity, and any particle entering Si.

The CSVs are split by Geant4 worker to avoid concurrent writes. Merge them by reading every matching `*_r*_t*.csv` file, not by taking one worker file. Each worker writes its own CSV files with headers.

CSV headers in column order

- `events` — `run_id,event_id,primary_entered_Al,primary_entered_cavity,primary_energy_at_cavity_MeV,proton_inelastic_in_Al,neutrons_created_in_Al,neutrons_entered_Si,Si_edep_MeV,primary_proton_Si_edep_MeV,direct_neutron_Si_edep_MeV`
- `reactions` — `run_id,event_id,track_id,parent_id,particle,process,volume,pre_energy_MeV,post_energy_MeV,x_mm,y_mm,z_mm,secondaries_total,secondary_neutrons`
- `neutrons` — `run_id,event_id,parent_track_id,creator_process,energy_MeV,x_mm,y_mm,z_mm,dx,dy,dz`
- `crossings` — `run_id,event_id,track_id,parent_id,particle,from_volume,to_volume,energy_MeV,x_mm,y_mm,z_mm`

The `_MeV` and `_mm` suffixes identify energy and position units. The `dx,dy,dz` columns are dimensionless direction cosines. In the neutron table, `creator_process` records the process that defined the parent step.

`primary_entered_cavity` indicates whether the primary proton reached the cavity from AlCase or, at 0 mm, from World. The energy column stores the most recent qualifying entry energy, or `-1` when there was no entry. The beam is limited to the 10 × 10 mm chip footprint. Neutron entries into silicon count crossing occurrences, including repeat entries by the same track.

`direct_neutron_Si_edep_MeV` is energy left directly on neutron steps; it is not a complete attribution of energy deposited by all neutron descendants. The total `Si_edep_MeV` includes all tracks.

## Installation

From your **host** shell:

```bash
cd ~/projects/geant-learn
unzip -o ~/Downloads/lunar-logging-patch.zip
```

Adjust the ZIP path to wherever you saved it. The overlay modifies only `geant4-ws/LunarDevice/src/ActionInitialization.cc`, and adds six new source/header files plus analysis scripts/macros. Back up or commit your work before overlaying. No change to your Docker files is required.

In your **Geant4 container** (assuming `~/geant4-ws/LunarDevice` is mounted):

```bash
cd ~/geant4-ws/LunarDevice
cmake -S . -B build -G Ninja
cmake --build build -j 4
cd build
./exampleB1 ../macros/run_log_smoke.mac
python3 ../scripts/analyze.py --dir results
```

If the build cannot locate Geant4, add `-DGeant4_DIR=/actual/path/to/dir/containing/Geant4Config.cmake` at CMake configuration time. If the build directory already exists and uses Ninja, the CMake command is generally sufficient.

For 10,000 events:

```bash
./exampleB1 ../macros/run_log.mac
python3 ../scripts/analyze.py --dir results
```

Results default to the *process working directory*, normally `LunarDevice/build/results/`. Override with `LUNAR_OUTPUT_DIR=/some/path` in your Docker shell. Use a fresh output directory for each process. The recorder reserves it with a `.lunar-run` marker at the first event and rejects directories that already have that marker or LunarDevice CSVs.

## Interaction visualization

Keep using your working Qt/TSG wireframe visualization macros. A particle-ID trajectory model can color neutrons yellow:

```text
/vis/modeling/trajectories/create/drawByParticleID
/vis/modeling/trajectories/drawByParticleID-0/set proton cyan
/vis/modeling/trajectories/drawByParticleID-0/set neutron yellow
/vis/scene/endOfEventAction accumulate
```

Only configure this **once** in a fresh viewer, and replace any `drawByCharge` model in that session rather than stacking duplicate models. For an individual nuclear event, `/tracking/verbose 2` followed by `/run/beamOn 1` may show relevant processes such as `protonInelastic` (most 100 MeV protons traversing a thin wall will not experience such a reaction).

## Physics / limitations

This is educational instrumentation for your Geant4 simulation. The active hadronic physics list must include relevant proton inelastic and neutron transport processes (`QBBC` in the official B1 main example, unless you changed it). `neutrons` counts only neutrons produced by processes **within AlCase**. Neutrons from other volumes and nuclear reactions from particles other than protons are not included in the `reactions` tally. Models, production cuts, geometry, source fluence, and validation matter for quantitative conclusions.

No lunar spectrum, real time normalization, SEE probability, or semiconductor charge collection is implemented.

## Known constraints

- The original `CMakeLists.txt` in official B1 uses `file(GLOB sources .../src/*.cc)`; if your customized `CMakeLists.txt` lists sources explicitly, add `src/LunarRecorder.cc` manually.
- The code uses C++17 `<filesystem>`, so ensure the CMake target uses C++17 or later.
- New files are intentionally named `Lunar*` to avoid overwriting existing B1 action implementations.

## Aluminum thickness sweep

Set `LUNAR_AL_THICKNESS_MM` to specify wall thickness in mm. The default is 2 mm. Use 0 for an unshielded reference with the vacuum cavity directly inside the world and no aluminum volume. For positive values, the outer enclosure is 100 × 100 × 30 mm and each cavity dimension decreases by twice the thickness. The thickness must be below 14.75 mm to leave clearance for the centered 0.5 mm chip.

From the application build directory, after building the executable:

```bash
python3 ../scripts/sweep_al.py --thicknesses 0 1 2 5 10
```

Each case gets a fresh directory under `sweep_results` (or `--outdir`). The runner prints each path and saves its batch log there. Pass the printed case path to `python3 ../scripts/analyze.py --dir`. Use repeated seeds and comparable event counts to estimate sampling uncertainty.
