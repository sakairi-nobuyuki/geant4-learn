# LunarDevice

## Overview

LunarDevice is an educational C++ application derived from Geant4 Basic Example B1. It simulates particle transport through shielding, secondary particle production, and energy deposition in a simplified semiconductor device. The geometry and beam are suitable for controlled Monte Carlo exercises. A validated lunar radiation environment model would require a representative source spectrum and independent physical validation.

## Simulation geometry

[DetectorConstruction.cc](src/DetectorConstruction.cc) constructs centered, axis-aligned boxes using Geant4 NIST materials. Dimensions below are full lengths in world X × Y × Z.

| Physical volume | Material | Dimensions | Configuration |
| --- | --- | --- | --- |
| `World` | `G4_Galactic` (vacuum) | 600 × 600 × 600 mm | Fixed |
| `AlCase` | `G4_Al` | 100 × 100 × 30 mm outer dimensions | Fixed outer dimensions. Present for positive wall thickness. |
| `Cavity` | `G4_Galactic` | 96 × 96 × 26 mm at the default thickness | Each outer dimension minus twice the wall thickness |
| `Chip` | `G4_Si` | 10 × 10 × 0.5 mm | Fixed at the origin. Silicon scoring volume. |

For positive thickness, the placement hierarchy is `World → AlCase → Cavity → Chip`. Set `LUNAR_AL_THICKNESS_MM` before launching the process to select the uniform wall thickness. The default is 2 mm. Positive values must be below 14.75 mm to leave clearance for the chip. At 0 mm, the 100 × 100 × 30 mm cavity is placed directly in the world and the aluminum volume is omitted. Geometry construction enables overlap checks.

## Particle source

[PrimaryGeneratorAction.cc](src/PrimaryGeneratorAction.cc) generates one 100 MeV proton per event. It samples X and Y independently and uniformly from −5 mm to +5 mm at world Z = +100 mm. Every proton travels in direction `(0, 0, -1)`, normal to the enclosure face and toward the chip footprint. This monoenergetic, unidirectional beam samples a small area and omits the energy, species, and angular distributions of lunar cosmic radiation.

## Physics configuration

[exampleB1.cc](exampleB1.cc) selects the **QBBC** reference physics list. Geant4 handles charged-particle ionization and multiple scattering, hadron–nucleus elastic and inelastic interactions, and secondary production and transport. For example, proton inelastic interactions can produce neutrons and other secondaries. Enabled processes are model choices whose applicability must be assessed for the experiment. [Geant4 QBBC reference](https://geant4.web.cern.ch/documentation/dev/plg_html/PhysicsListGuide/reference_PL/QBBC.html)

The headless smoke tests used Geant4 11.4.3 in multithreaded mode with the QBBC default production range cut of 0.7 mm. The installation supplies Geant4 datasets such as G4EMLOW, G4PARTICLEXS, and PhotonEvaporation. Use `geant4-config --datasets` from the installation's `bin` directory to list configured dataset versions and paths.

Application scoring is separate from physics. [SteppingAction.cc](src/SteppingAction.cc) scores energy deposited on steps in the chip, [EventAction.cc](src/EventAction.cc) totals it per event, and [RunAction.cc](src/RunAction.cc) merges worker totals and divides by chip mass to report absorbed dose in gray (J/kg). The result is dose for the simulated run. A lunar dose rate requires spectrum, angular distribution, area, flux, and time normalization. A semiconductor single-event effect (SEE) probability requires a device-response model, including sensitive volumes and charge collection.

## Build instructions

From `LunarDevice/` inside the existing development container, build the configured Ninja tree:

```bash
cmake --build build
```

If the build tree needs to be configured, use the installed Geant4 prefix. The following uses the default prefix for this container. Set `geant4_prefix` to the actual prefix if it differs.

```bash
geant4_prefix="$HOME/geant4-install"
source "$geant4_prefix/bin/geant4.sh"
cmake -S . -B build -G Ninja \
  -DGeant4_DIR="$geant4_prefix/lib/cmake/Geant4"
cmake --build build
```

[CMakeLists.txt](CMakeLists.txt) finds Geant4 with `ui_all` and `vis_all`, compiles the application target `exampleB1`, and copies the startup macros into `build/`. The logging, LunarDevice batch, and visualization preset macros used below are referenced from the source tree. Building the existing tree was verified. The configuration command follows the current CMake cache and project rules.

## Running simulations

From `LunarDevice/`, source the installed runtime setup as above. Use a new output directory for each process. This verified headless smoke-test workflow sets Qt to offscreen mode and runs the 100-event logging macro with no enclosure override.

```bash
out=$(mktemp -d)
env -u LUNAR_AL_THICKNESS_MM QT_QPA_PLATFORM=offscreen LUNAR_OUTPUT_DIR="$out" \
  ./build/exampleB1 macros/run_log_smoke.mac >"$out/run.log" 2>&1
printf '%s\n' "$out"
```

| Batch macro | Commands and verification |
| --- | --- |
| [macros/run_log_smoke.mac](macros/run_log_smoke.mac) | Initializes and runs 100 events. Verified with all four CSV families and 100 unique event rows per run. |
| [run_lunar.mac](run_lunar.mac) | Basic batch macro. Requests a 10 µm production range cut and two runs of 10,000 events each. Inspected in source. Execution and application of the requested cut were not verified. |
| [macros/run_log.mac](macros/run_log.mac) | Initializes and requests one 10,000-event run with reduced event and tracking verbosity. Inspected in source. The 10,000-event execution was not verified. |

To launch the basic or larger logging run, use the same shell pattern with a fresh `out=$(mktemp -d)` and pass `run_lunar.mac` or `macros/run_log.mac` as the executable's macro argument. The logger runs in both interactive and batch mode. Check `run.log` for Geant4 command errors and the processed event count before using the CSVs.

## CSV output

[LunarRecorder.cc](src/LunarRecorder.cc) opens files when a worker starts its first event of a run. It uses `LUNAR_OUTPUT_DIR`, or `results/` relative to the working directory if unset or empty. Names end in `_r<run ID>_t<Geant4 thread ID>.csv`. For one case, read every file of each family with a CSV parser and retain the run and event IDs plus the worker ID from the filename if needed. Keep independent processes in separate directories because their IDs can repeat. Use event rows, including zero-deposit events, as the denominator for per-primary rates. Reaction, birth, and crossing files can contain just a header.

Run/thread filenames repeat across invocations and would be overwritten if directory reuse were allowed. The current recorder rejects existing LunarDevice CSVs and reserves each directory with a `.lunar-run` marker. Use a fresh directory even after an interrupted run.

Headers below are in file order. Fields ending in `_MeV` contain energies in MeV and fields ending in `_mm` contain world positions in mm. IDs, counts, and flags are unitless. The neutron direction components are dimensionless.

### `events_r*_t*.csv`

One row per completed event, with flags for primary entry into the enclosure and cavity, counts for selected reactions and neutron activity, and energy deposition in silicon.

```text
run_id,event_id,primary_entered_Al,primary_entered_cavity,primary_energy_at_cavity_MeV,proton_inelastic_in_Al,neutrons_created_in_Al,neutrons_entered_Si,Si_edep_MeV,primary_proton_Si_edep_MeV,direct_neutron_Si_edep_MeV
```

The cavity flag counts entry from `AlCase` or, at 0 mm, `World`. It measures access to the cavity only. The stored energy is the most recent qualifying entry's kinetic energy, or −1 MeV if there was no entry. Neutron entries into silicon are crossing occurrences and may count the same track repeatedly. `Si_edep_MeV` includes all tracks. `direct_neutron_Si_edep_MeV` sums neutron steps in silicon and **excludes deposition by neutron-generated descendants**, including charged tracks.

### `reactions_r*_t*.csv`

One row per `protonInelastic` step with its pre-step volume equal to `AlCase`. Rows include primary and secondary protons, their pre- and post-step kinetic energies, the post-step position, and secondary counts for that step. Coverage is limited to this process, particle, and volume.

```text
run_id,event_id,track_id,parent_id,particle,process,volume,pre_energy_MeV,post_energy_MeV,x_mm,y_mm,z_mm,secondaries_total,secondary_neutrons
```

### `neutrons_r*_t*.csv`

One row per neutron secondary produced on a step starting in `AlCase`, from any parent species. Records include initial kinetic energy, birth position, and initial momentum direction. `creator_process` contains the process that defined the parent step. The row carries the parent track ID, but has no neutron track ID for linking it directly to subsequent crossings or descendant tracks.

```text
run_id,event_id,parent_track_id,creator_process,energy_MeV,x_mm,y_mm,z_mm,dx,dy,dz
```

### `crossings_r*_t*.csv`

One row per selected geometrical boundary crossing, with post-step kinetic energy and position. The recorder selects primary protons entering `AlCase` from `World`, primary protons entering `Cavity` from `AlCase` or `World`, and any particle entering `Chip`. Repeated entries can produce several rows for one track. Cavity entry alone does not measure transmission out of the enclosure.

```text
run_id,event_id,track_id,parent_id,particle,from_volume,to_volume,energy_MeV,x_mm,y_mm,z_mm
```

## Visualization

Use a Qt-capable Geant4 installation and a working graphical display. From `LunarDevice/build/`, start the application with no macro argument. Use a shielded configuration with these presets because they refer to `AlCase`.

```bash
out=$(mktemp -d)
G4VIS_DEFAULT_DRIVER=TSG LUNAR_AL_THICKNESS_MM=2 LUNAR_OUTPUT_DIR="$out" ./exampleB1
```

| Macro | Responsibility |
| --- | --- |
| [init_vis.mac](init_vis.mac) | Runs automatically at interactive startup, initializes Geant4, and executes `vis.mac`. |
| [vis.mac](vis.mac) | Opens the selected viewer once, draws the geometry, and configures smooth, accumulated trajectories with the active charge-based model. It still contains legacy B1 text labels `Shape1` and `Shape2` that do not describe this geometry. |
| [macros/vis_wireframe.mac](macros/vis_wireframe.mac) | Hides world and cavity, forces the gray enclosure to wireframe, and displays the chip in green with surface rendering. |
| [macros/vis_chip_only.mac](macros/vis_chip_only.mac) | Hides world, cavity, and enclosure and displays the green chip in surface mode. Trajectories can still appear. |

At the Geant4 prompt, choose one display preset, then run a few events to see trajectories. The two preset commands are alternatives.

```text
/control/execute ../macros/vis_wireframe.mac
/control/execute ../macros/vis_chip_only.mac
```

```text
/run/beamOn 5
```

For explicit proton and neutron colors, optionally customize the trajectory-model block in `build/vis.mac` after building, then start a fresh session from `build/`. CMake can restore the source version of this generated copy when it reconfigures. Replace the three active `drawByCharge` lines with the following commands. Keep the existing trajectory scene and accumulation commands.

```text
/vis/modeling/trajectories/create/drawByParticleID
/vis/modeling/trajectories/drawByParticleID-0/set proton cyan
/vis/modeling/trajectories/drawByParticleID-0/set neutron yellow
```

Qt/TSG wireframe rendering was demonstrated previously. A TSG segmentation fault was also reported during redraw after changing visualization attributes. Use wireframe if transparency is unstable. Apply presets to the existing scene and avoid repeated viewer setup, duplicate trajectory models, or extra scene notifications. Neither the optional color customization nor the interactive presets were rerun for this documentation update. A short display run may contain no visible neutron tracks.

## Existing analysis scripts

Both scripts use the Python standard library. From `LunarDevice/build/`, use the printed simulation output path with the analyzer, or run a thickness sweep:

```bash
python3 ../scripts/analyze.py --dir "$out"
python3 ../scripts/sweep_al.py --macro ../macros/run_log_smoke.mac --thicknesses 0 2 5 10
```

- [analyze.py](scripts/analyze.py) reads all matching event and neutron CSVs in a directory. It prints entry fractions, reaction and neutron counts, energy deposition, dose, and basic neutron energy statistics. It pools matching run IDs within the directory and assumes a 100 MeV source and a 116.5 mg chip. It supplies no plotting or uncertainty estimates. Its summary behavior was inspected in source.
- [sweep_al.py](scripts/sweep_al.py) runs one process per thickness, sets `LUNAR_AL_THICKNESS_MM` and `LUNAR_OUTPUT_DIR`, and saves `run.log` in a unique case directory under `sweep_results/` by default. Without `--macro`, it uses `../macros/run_log.mac`. The current detector **does read** the thickness variable. Previous 100-event verification used the runner with a diagnostic macro and confirmed geometry changes at 0, 2, 5, and 10 mm. The runner provides no seed management, automatic analysis, or general experiment schema.

## Current capabilities and limitations

| Status | Capability |
| --- | --- |
| Implemented and verified | Existing CMake/Ninja build, 100-event headless transport and silicon scoring, four worker-local CSV families with validated event counts and headers, 0/2/5/10 mm geometry and overlap checks, unique sweep directories, and rejection of reused output directories without changing existing CSVs. |
| Implemented but unverified in recent tests | The 10,000-event logging macro, the basic batch macro's cut and two-run sequence, and the analyzer's summaries. Qt/TSG wireframe rendering has historical evidence. The current interactive presets and optional particle colors were not rerun. |
| Known defects or limitations | Legacy B1 labels in the startup view, reported TSG redraw instability, repeat crossings in entry counts, direct neutron-step scoring only, and a simplified beam with no lunar flux normalization or device-response model. |
| Planned | General experiment configuration, richer geometry and source models, ancestry tracking, and automated analysis and captures. |

## Future development

Planned extensions include validated YAML experiment definitions and broader Python orchestration beyond the current thickness loop, material and geometry studies beyond the existing aluminum setting, neutron ancestry tracking, and automated CSV validation, analysis, and plotting. Other proposed work includes testing `TSG_OFFSCREEN` image capture, importing validated STEP/CAD-derived geometry, and using realistic lunar radiation source spectra with physical normalization. See the project design notes. [Architecture](../architecture.md)

## References

- [Geant4 Book for Application Developers](https://geant4.web.cern.ch/documentation/dev/bfad_html/ForApplicationDevelopers/index.html)
- [Geant4 Basic Examples, including B1](https://geant4.web.cern.ch/documentation/dev/bfad_html/ForApplicationDevelopers/Examples/BasicCodes.html)
- [LunarDevice logging notes](README_LOGGING.md)
