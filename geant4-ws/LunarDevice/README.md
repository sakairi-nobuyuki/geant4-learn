# LunarDevice

## Overview

LunarDevice is an educational C++ application derived from Geant4 Basic Example B1. It simulates particle transport through shielding, secondary particle production, and energy deposition in a simplified semiconductor device. The geometry and beam are suitable for controlled Monte Carlo exercises. A validated lunar radiation environment model would require a representative source spectrum and independent physical validation.

## View geometry

With the Geant4 runtime configured and a working graphical display, start the viewer with no arguments.

```bash
cd /home/app/geant4-ws/LunarDevice/build
./lunar_device
```

Startup runs `init_vis.mac`, which initializes Geant4 and calls `vis.mac` to open the Qt/TSG viewer. The startup view shows the aluminum enclosure in wireframe and the silicon chip in green, with the world and vacuum cavity hidden. Opening the viewer generates zero Monte Carlo events and creates no CSVs. You can open it with saved CSVs already in `results/`.

Enter these commands at the Geant4 prompt to set a camera angle and zoom in.

```text
/vis/viewer/set/viewpointThetaPhi 65 35
/vis/viewer/zoom 1.5
```

To show only the chip or restore the enclosure, execute the corresponding preset from the open viewer. The enclosure preset is for positive aluminum thickness.

```text
/control/execute ../macros/vis_chip_only.mac
/control/execute ../macros/vis_wireframe.mac
```

`/control/execute ../vis_lunar.mac` also applies the wireframe preset to the open viewer. Qt/TSG startup and three consecutive interactive runs of 5, 10, and 20 events were verified on the container display.

## Run Monte Carlo

Run these commands from `LunarDevice/build/`. With no macro, the application waits at the Geant4 prompt. Enter `/run/beamOn N` to simulate N primary events. You can issue that command repeatedly in the same session.

| Command | Output behavior |
| --- | --- |
| `./lunar_device` | Overwrite disposable results in `results/res/` when the first event starts |
| `./lunar_device --target experiment_001` | Save a named experiment in `results/experiment_001/` |
| `./lunar_device --target` | Generate a unique experiment directory |
| `./lunar_device --macro run_lunar.mac` | Execute a batch macro with default output |
| `./lunar_device --target --macro run_lunar.mac` | Execute a batch macro with a unique output directory |

The table assumes `LUNAR_OUTPUT_DIR` is unset. CMake copies `run_lunar.mac` into the build directory. Use `../macros/run_log_smoke.mac` for a headless 100-event smoke test. The baseline macro runs two sets of 10,000 events with a 10 µm production range cut.

A macro can be positional. For example, use `./lunar_device ../run_lunar.mac`, `./lunar_device --target experiment_001 ../run_lunar.mac`, or `./lunar_device --target ../run_lunar.mac`. Bare `--target` generates a name at the end of the command line or before another option or a positional filename ending in `.mac`. Otherwise the next argument is the target name. Use `--target=NAME` for a name ending in `.mac`, and `--macro FILE` for a macro with any extension.

The application prints the output directory at startup. It selects it in this order.

1. `--target` selects a protected directory under the project’s `results/`. Named targets start with an ASCII letter or digit and may also contain dots, underscores, and hyphens. The name `res` is reserved for disposable output.
2. A nonempty `LUNAR_OUTPUT_DIR` selects a protected directory for compatibility with the sweep runner. Relative values resolve from the working directory. The disposable directory is reserved for default mode.
3. With neither setting, the default is `LunarDevice/results/res/`, anchored to the project source directory configured by CMake.

Protected directories may already contain unrelated files such as a sweep log. Existing simulation CSVs cause startup to fail. A directory lock prevents two LunarDevice processes from using the same output directory at once.

## Analyze saved results

From `LunarDevice/build/`, run the analyzer with no options for disposable results, or pass a saved directory with `--dir`.

```bash
python3 ../scripts/analyze.py
python3 ../scripts/analyze.py --dir ../results/experiment_001
```

The analyzer reads all matching event and neutron worker CSVs, pools run IDs within the input directory, and prints statistics to standard output. Rerun the command to regenerate the summary. It uses only the Python standard library and creates no files. The summary includes entry fractions, reaction and neutron counts, silicon energy deposition, absorbed dose, and neutron energy statistics when birth records are available. The calculation assumes a 100 MeV proton source and a 116.5 mg chip.

Missing or malformed event input is reported as an error. Inconsistent or missing neutron files produce a warning and omit neutron energy statistics. The original event count must be checked against the simulation log because the CSVs do not record the requested count. Plot generation is outside the current analyzer.


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

[CMakeLists.txt](CMakeLists.txt) finds Geant4 with `ui_all` and `vis_all`, compiles the application target `lunar_device`, and copies `init_vis.mac`, `vis.mac`, and `run_lunar.mac` into `build/` using `configure_file(COPYONLY)`. Edit the source macros and run `cmake --build build -j 4` to refresh the copies through CMake regeneration. The executable reads the startup macros relative to its working directory. Commands starting with `../` use the source copies of the macros.

CMake also copies the optional B1 macros `run1.mac` and `run2.mac`. They override the default particle type and energy, so use the LunarDevice macros above for the standard 100 MeV proton runs.

## CSV output

Each worker opens its files at its first event and flushes and closes them at the end of its run. Filenames include the Geant4 run and worker IDs. Each new process starts again at run zero. Read all worker files in the selected directory and use all event rows, including zero-deposit events, as the denominator for per-primary rates. Reaction, birth, and crossing files may contain only a header. [LunarRecorder.cc](src/LunarRecorder.cc)

At its first event, a default-mode process removes regular CSVs matching the four LunarDevice filename formats in `results/res/`. This clears previous run and worker IDs, including IDs the new process may never use. Later runs in the same process keep their own files. Starting the viewer and exiting before a simulation leaves existing CSVs untouched. Analyze results after the runs of interest complete.

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

## Aluminum thickness sweeps

From `LunarDevice/build/`, the existing sweep script runs one batch process per thickness and logs each case in a fresh directory under `sweep_results/` by default.

```bash
python3 ../scripts/sweep_al.py --macro ../macros/run_log_smoke.mac --thicknesses 0 2 5 10
```

Pass a printed case directory to the analyzer for its summary. For the CSV schema and scoring details, see [the logging notes](README_LOGGING.md).

## Future development

Planned extensions include validated YAML experiment definitions and broader Python orchestration beyond the current thickness loop, material and geometry studies beyond the existing aluminum setting, neutron ancestry tracking, and automated CSV validation, analysis, and plotting. Other proposed work includes testing `TSG_OFFSCREEN` image capture, importing validated STEP/CAD-derived geometry, and using realistic lunar radiation source spectra with physical normalization. See the project design notes. [Architecture](../architecture.md)

## References

- [Geant4 Book for Application Developers](https://geant4.web.cern.ch/documentation/dev/bfad_html/ForApplicationDevelopers/index.html)
- [Geant4 Basic Examples, including B1](https://geant4.web.cern.ch/documentation/dev/bfad_html/ForApplicationDevelopers/Examples/BasicCodes.html)
- [LunarDevice logging notes](README_LOGGING.md)
