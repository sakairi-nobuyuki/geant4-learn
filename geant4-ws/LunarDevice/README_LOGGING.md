# LunarDevice particle-interaction logging

This overlay extends the **existing** Geant4 B1-derived `LunarDevice` source. It does not replace your customized detector geometry, source, visualization, Dockerfile, or the original B1 dose calculation.

## What is recorded

- `events_r<run>_t<worker>.csv`: one row per primary event, energy deposition in silicon, proton boundary crossings, nuclear reaction and neutron counts.
- `reactions_r<run>_t<worker>.csv`: `protonInelastic` steps **inside AlCase** (primary and secondary proton tracks).
- `neutrons_r<run>_t<worker>.csv`: neutrons **born inside AlCase**, with source process, position, direction and initial kinetic energy.
- `crossings_r<run>_t<worker>.csv`: selected geometry-boundary crossings: primary proton entering Al, primary proton entering cavity, and any particle entering Si.

The CSVs are split by Geant4 worker to avoid concurrent writes. Merge them by reading every matching `*_r*_t*.csv` file, not by taking one worker file. Each worker writes its own CSV files with headers.

`primary_entered_cavity` is **not** a general attenuation coefficient or transmission through the entire Al enclosure: it is the first-passage *event indicator* for the Al -> Cavity boundary (primary proton), and the current beam is artificially limited to the 10x10 mm chip footprint. The `events` CSV also records neutron entries into silicon, which are *crossing counts* and can include the same track more than once.

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

Results default to the *process working directory*, normally `LunarDevice/build/results/`. Override with `LUNAR_OUTPUT_DIR=/some/path` in your Docker shell. Output files for a repeated run use the same names and **are overwritten**, so archive runs or change the output directory before rerunning.

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

- **Runtime compilation was not possible here**: your complete `LunarDevice` source tree and installed Geant4 SDK are not available in this environment. Changes are based on Geant4 11.4 B1 APIs and must be compiled/tested in your Docker container.
- The original `CMakeLists.txt` in official B1 uses `file(GLOB sources .../src/*.cc)`; if your customized `CMakeLists.txt` lists sources explicitly, add `src/LunarRecorder.cc` manually.
- The code uses C++17 `<filesystem>`, so ensure the CMake target uses C++17 or later.
- New files are intentionally named `Lunar*` to avoid overwriting existing B1 action implementations.

## Optional aluminum thickness sweep

An OPTIONAL alternative geometry is provided at `extras/DetectorConstruction_parametric.cc` **outside** the source overlay. It does not touch your existing geometry unless you explicitly copy it:

```bash
# From the HOST repo root; after extracting the ZIP:
cp extras/DetectorConstruction_parametric.cc \
   geant4-ws/LunarDevice/src/DetectorConstruction.cc
```

Confirm that this simplified geometry is appropriate before replacing any custom model. The thickness is read from `LUNAR_AL_THICKNESS_MM` at geometry construction; default is 2 mm. At 0 mm, `AlCase` is absent and `Cavity` is directly inside `World`. Source and scoring volumes remain identical across cases. It can be swept without recompiling each thickness, but C++ must first be rebuilt after installing the parametric geometry.

```bash
cd ~/geant4-ws/LunarDevice
cmake -S . -B build -G Ninja && cmake --build build -j 4
cd build
python3 ../scripts/sweep_al.py --thicknesses 0 1 2 5 10
python3 ../scripts/analyze.py --dir sweep_results/al_2mm
```

The logger counts World->Cavity as a cavity entry for the 0-mm case. For an apples-to-apples comparison, use the same source particle distribution, the same number of primaries, and repeated seeds/runs for statistical uncertainties. The sweep currently provides raw event results per case; automatic uncertainty bars and dose-rate normalization are not included.
