# Architecture and Implementation Reference — geant-learn

For installation, startup, and everyday commands, see [README.md](README.md). For AI coding constraints and verification requirements, see [AGENTS.md](AGENTS.md).

## 1. Purpose and status

`geant-learn` is an educational Geant4 project for studying particle transport through shielding and energy deposition in a simplified semiconductor device. It uses a Dockerized Ubuntu environment and a B1-derived C++ application named `LunarDevice`.

**Document status:** This is a design reference based on the maintainer's reported layout and demonstrated runs, not an audit of the live repository. It separates **observed or demonstrated behavior**, **optional/unverified patch work**, and **planned architecture**. Inspect the checkout before implementing an interface described here.

### Demonstrated baseline

- A world volume, aluminum box-shaped enclosure, vacuum cavity, and central silicon chip are constructed in Geant4.
- A simplified approximately 100 MeV monoenergetic primary proton beam is simulated.
- Geant4 reports silicon energy deposition, scoring-volume mass, and absorbed dose.
- The Qt/TSG visualization has displayed the enclosure and trajectories; a wireframe enclosure is the preferred stable presentation.
- The application builds from a B1-derived CMake project inside a Docker-backed development setup.

These are not yet verified measurements of actual lunar dose rates or semiconductor SEE probability.

### Optional patch / not assumed installed

Earlier development supplied an optional logging and aluminum-thickness-sweep patch, reportedly containing worker-local event, reaction, neutron, and crossing CSV files and Python aggregation scripts. Its integration, exact field names, compilation, and physical correctness must be verified against the current checkout.

### Planned, not yet implemented as a unified system

Validated YAML experiment definitions; a general material/geometry sweep engine; normalized lunar source spectra; automatic offscreen captures; robust source/version manifests; STEP conversion and GDML/tessellated-solid loading; secondary-particle ancestry attribution in silicon; semiconductor SEE response models.

## 2. Design policy

Prefer small, inspectable components and explicit contracts. Protect the working Geant4 core. Keep geometry, source, physics, scoring, orchestration, and reporting independently changeable where possible. No large rewrite is justified merely to introduce abstractions.

The scientific boundary is essential:

**Executable works** != **numerical outputs are consistent** != **physics model is validated** != **mission-qualified prediction**.

## 3. Responsibility boundaries

| Layer | Responsibility | Must not own |
| --- | --- | --- |
| Host Ubuntu/WSL | Development shell, Git, Docker invocation, optional display server | Particle physics |
| Docker Compose / Dockerfile | Reproducible dependencies, Geant4 build/runtime, mounts, non-root execution | Scientific interpretation or case selection |
| Geant4 C++ application | Geometry/material construction; primary sources; physics list; tracking; scoring; recording | Experiment-grid expansion or dashboard/report generation |
| Geant4 macros / UI | Supported runtime commands, batch runs, interactive visualization | Unimplemented YAML parsing or complex statistics |
| Python runner (planned) | Validate config, enumerate cases, invoke one Geant4 process per case, collect artifacts | Particle interaction physics |
| Python analysis (optional/planned) | Merge output, normalize rates, plot uncertainty/spectra, prepare comparisons | Modifying Geant4 physics implicitly |
| CAD conversion (planned) | Convert/check STEP-derived geometry and material mappings | Automatically certify geometry/physics fidelity |

For a parameter sweep, the default target is one independent, short-lived Geant4 process per resolved case. This avoids subtle cross-run state carryover from geometry reinitialization and eases failure isolation.

## 4. Current source tree

Reported repository layout (omitting generated internals):

```text
geant-learn/
├── README.md
├── AGENTS.md                 # This project's coding guidance
├── architecture.md          # This document
├── docker/
│   └── Dockerfile
├── docker-compose.yaml
└── geant4-ws/
    ├── B1-build/             # Generated build artifacts
    └── LunarDevice/
        ├── CMakeLists.txt
        ├── exampleB1.cc
        ├── include/
        │   ├── DetectorConstruction.hh
        │   ├── PrimaryGeneratorAction.hh
        │   ├── ActionInitialization.hh
        │   ├── RunAction.hh
        │   ├── EventAction.hh
        │   └── SteppingAction.hh
        ├── src/
        │   ├── DetectorConstruction.cc
        │   ├── PrimaryGeneratorAction.cc
        │   ├── ActionInitialization.cc
        │   ├── RunAction.cc
        │   ├── EventAction.cc
        │   └── SteppingAction.cc
        ├── init_vis.mac
        ├── vis.mac
        ├── vis_lunar.mac
        ├── run_lunar.mac
        ├── macros/
        │   ├── vis_wireframe.mac
        │   └── vis_chip_only.mac
        └── build/            # Generated build artifacts
```

Legacy B1 files such as `GNUmakefile`, `exampleB1.in`, `exampleB1.out`, and `run1.mac`/`run2.mac` may also remain. Do not delete them casually. The source tree, not a generated build directory, is authoritative for version-controlled macros.

## 5. Physical model and terminology

### Simplified geometry

The reported baseline uses:

| Physical volume | Material | Overall dimensions |
| --- | --- | --- |
| World | `G4_Galactic` | 600 × 600 × 600 mm |
| AlCase | `G4_Al` | 100 × 100 × 30 mm |
| Cavity | `G4_Galactic` | 96 × 96 × 26 mm |
| Chip | `G4_Si` | 10 × 10 × 0.5 mm |

The nested cavity represents a nominal 2 mm enclosure wall. The chip is centered; the baseline primary beam propagates approximately along the negative Z direction through the top wall. With a 10 × 10 mm sampled beam area, the baseline measures a targeted centerline response, not whole-spacecraft shielding performance.

### Events, tracks, steps, and physics

- **Event:** one generated primary event and all its descendants.
- **Track:** one particle history within an event, with a track ID and parent ID.
- **Step:** the particle's incremental transport and interaction, including deposited energy and the process defining the step.
- **Physics List:** registered electromagnetic/hadronic processes and models; inspect `exampleB1.cc` to confirm the actual selected list. B1's reference configuration may use `QBBC`, while alternative lists were discussed; do not infer the active list from a prior suggestion.
- **Secondary neutron:** a neutron produced by a hadronic interaction, possibly in aluminum, silicon, or other volumes. Its presence and genealogy should be derived from Geant4 track/step data.

A primary proton may transit the housing, deposit energy, scatter, undergo a hadronic reaction, and produce secondaries. The same event can contain charged products, neutrons, gammas, and subsequent reactions. Visual track colors are diagnostic, not an interaction-rate estimate.

### Current scoring

The B1-derived baseline accumulates energy deposited in the silicon scoring volume and divides by its mass to report absorbed dose. In a sample 10,000-primary run the reported mass was approximately 116.5 mg, consistent with the stated Si dimensions. The total dose is for that simulated source population only.

No physically normalized lunar dose rate or SEE/SEU/SEL probability follows directly from the baseline output. Proper flux normalization, sensitive-volume assumptions, and device response models are separate tasks.

## 6. Execution flows

### Current batch flow

```text
B1-derived source + run_lunar.mac
    -> CMake/Ninja executable lunar_device
    -> Geant4 primary generation
    -> geometry navigation and particle transport
    -> energy-deposition scoring
    -> run summary (stdout/log) and worker-local CSVs
```

### Current interactive viewing flow

```text
lunar_device in interactive mode
    -> init_vis.mac
    -> vis.mac opens a viewer and configures scene/tracks
    -> optional vis_wireframe.mac or vis_chip_only.mac changes visibility
    -> /run/beamOn N
    -> TSG viewer shows geometry and available trajectories
```

The wireframe/camera macros are meant for an already initialized interactive session and should not open additional viewers or add duplicate trajectory models. The old B1 `Envelope`, `Shape1`, and `Shape2` names are not part of the current geometry. Transparency redraw and repeated scene notifications have previously caused or accompanied a segmentation fault; preserve a minimal working view as the fallback.

### Target experiment flow (planned)

```text
standalone experiment.yaml
    -> Python schema/unit validation
    -> deterministic case enumeration and resolved configurations
    -> macro/input generation
    -> one Geant4 process per case (batch)
    -> thread-local CSVs, logs, metadata, exit status
    -> merge + statistical summaries + plots
    -> separate small-run visualization capture
    -> experiment manifest and comparison report
```

These Python/YAML interfaces do not currently exist as a verified single workflow.

## 7. Data and measurement contracts

### Raw output layers

The optional logging patch conceptually separates four tables. Their exact names and fields must be read from the installed code:

| Logical dataset | Grain | Intended interpretation |
| --- | --- | --- |
| Events | One primary event | Si total energy deposition, selected reaction/crossing counts, flags |
| Reactions | One recorded proton inelastic reaction in Al | Process and interaction location/kinematics |
| Neutrons | One selected neutron birth | Parent, creator process, initial energy, position, direction |
| Crossings | One selected boundary crossing | Particle and source/destination volumes, kinetic energy |

Worker-specific files, if used, require collection across all workers and stable run/event identity. A thread-local event number alone is not an experiment-wide unique identifier across independent cases.

### Derived metrics and denominators

- Mean Si energy deposition per primary: sum of *all* event-level deposits / number of generated primaries (including zero-deposit events).
- Secondary neutron yield: number of qualified neutron births / number of primaries, with material/process filters documented.
- First-entry cavity fraction: distinct primary tracks reaching the cavity / emitted primaries; not the same as complete shielding transmission.
- Complete transmission: explicitly define the downstream exit surface and which surviving tracks qualify.
- Si neutron contribution: specify direct neutron steps versus all descendant energy deposits; the latter requires provenance propagation.
- Report sample size, statistical uncertainty, and model assumptions. Do not conflate Monte Carlo error with systematic physical-model uncertainty.

### Experimental provenance (planned)

A durable case should record the full resolved config; source Git commit and dirty-state/source digest; Geant4 version/physics list/datasets; cuts; random seed; primary sampling/normalization; geometry/material mapping; command line; start/end timestamps; exit status; output checksums; and generated images. Repeated runs must not be silently treated as independent validation of a fixed model.

## 8. Proposed YAML and runner contract (future)

A future `configs/baseline.yaml` could define source, geometry, physics, scoring, event count, seed, output, and capture settings. Proposed sweep axes include enclosure material, wall thickness, external dimensions, chip material, chip placement, and source energy. **This is a proposed contract, not an implemented CLI or set of existing Geant4 commands.**

Design rules:

1. One standalone resolved config per case; no surprising implicit merge with defaults.
2. Validate supported axes, values, unit types, geometry constraints, and materials before launching any case.
3. Generate the Cartesian product only for axes explicitly marked as sweeps; bound case count.
4. Archive each resolved case and retain failed-case errors and exit status.
5. Distinguish a unique *case identity* from a particular *execution attempt* and an individual Geant4 *run*.
6. Start sequentially; enable safe bounded parallel execution only when needed.
7. Keep statistical runs separate from visualization runs, which typically use fewer events.
8. Parameter changes should not silently modify physics settings or detector sensitivity.

A thin `G4GenericMessenger`/`G4UImessenger` layer may expose supported geometry and source parameters to macros. Reinitialization rules and initialization order must be tested. No messenger command should be documented as callable until implemented.

## 9. Geometry roadmap and CAD/STEP

### Near-term geometry sweeps

Use Geant4 analytic solids (`G4Box`, `G4Tubs`, etc.) with configurable dimensions and materials. Change material or dimensions through supported parameterized construction without rewriting C++ per case. Validate positive wall thickness and cavity containment; zero-thickness/no-shield is a distinct geometry variant.

### STEP imports (future)

STEP is CAD boundary-representation geometry, not directly a Geant4 simulation description. A conversion stage must choose between analytic-solid reconstruction, tessellated solid/mesh import, and validated GDML construction. Material assignment and volume hierarchy often require additional metadata. Units, placement transforms, watertightness, topology quality, and overlaps must be checked.

Prefer a stable detector/geometry loader to generating and compiling a new `DetectorConstruction.cc` for every STEP file. Code generation remains an option only if it proves to be the smallest reliable path for a specific geometric subset.

## 10. Visualization and automated captures

Current known-good workflow: Qt/TSG, aluminum as gray wireframe, silicon as green solid, trajectories optionally colored by particle type. Avoid repeatedly adding trajectory models or opening viewers. `TSG_OFFSCREEN` may enable unattended PNG capture if supported by the built Geant4 installation, but this remains to be smoke-tested in the Docker image.

For reproducible future captures, record the graphics driver, resolution, camera, visibility/colors, random seed, event count, and output path. A clean geometry image and a small event-track image should be separate outputs for each case. Screenshot differences are not quantitative physical results.

## 11. Verification strategy

Verification is layered:

| Level | Checks | Claim allowed |
| --- | --- | --- |
| Build | CMake config, compiler/linker success | Application builds |
| Runtime smoke | Batch exits normally, no missing assets | Application runs |
| Data integrity | Complete events, defined units, finite scores, no MT races | Output is structurally consistent |
| Numerical checks | Dose = energy / mass, geometry overlap checks, reproducible baseline, cut/event-count convergence | Numerical behavior is plausible under stated settings |
| Physics validation | Comparison with trusted analytical limits, benchmark setups, reference measurements and uncertainty analysis | Model validity within an evaluated domain |

Keep tests small and headless by default. Production runs and Monte Carlo convergence studies are distinct from CI smoke tests. Capture failures should not invalidate an independently successful numerical batch run.

## 12. Prioritized next changes

1. **Audit current checkout**: identify installed patch files, active physics list, macro invocation flow, CSV schemas, and Docker command paths.
2. **Stabilize scoring/logging**: check event denominator, worker merges, reaction location, neutron genealogy, and unit conventions using a short batch run.
3. **Parameterize simple geometry/materials**: expose a minimal supported runtime interface, preserving the original baseline.
4. **Add bounded YAML/Python experiment runner**: standalone configs, one case/process, manifests, clear failures.
5. **Add analysis and capture**: normalized metrics, uncertainty plots, separate offscreen images.
6. **Add CAD geometry import**: simple STEP first, validation checks before complex assemblies.
7. **Advance physical model**: physically normalized lunar spectra, regolith secondaries, and ultimately device response/SEE modeling with separate validation evidence.

## 13. Out of scope for the initial educational baseline

A complete lunar GCR/SEP environment, mission-specific shielding qualification, transistor-level charge transport, SEE-rate guarantees, full CAD assemblies, and a real-time visual simulation service. These require additional physics inputs, substantial testing, and validation beyond a successful particle-transport demo.
