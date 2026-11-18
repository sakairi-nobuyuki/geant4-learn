# Coding Agent Instructions — geant-learn

## Purpose and scope

This repository is an educational Geant4 environment for Monte Carlo particle-transport experiments in an Ubuntu/Docker workflow. The current application, `geant4-ws/LunarDevice/`, is a modification of Geant4 Basic Example B1: a silicon chip inside an aluminum enclosure is irradiated by a simplified proton source.

Optimize for a **small, inspectable, reproducible research system**. Do not mistake a successful run for a validated lunar radiation prediction.

For operating instructions, see [README.md](README.md). For module responsibilities, boundaries, data semantics, and planned interfaces, see [architecture.md](architecture.md). These documents describe the repository as reported by its maintainer; inspect the actual checkout before relying on paths or assuming a feature is installed.

## Engineering rules

1. **Understand before changing.** Inspect the relevant C++, CMake, macros, Docker configuration, and tests before editing them.
2. **Preserve working behavior.** Keep the existing B1-derived geometry, 100 MeV proton baseline, dose scoring, and working TSG wireframe visualization unless the task explicitly changes them.
3. **Make the smallest coherent change.** Avoid speculative frameworks, broad rewrites, and unrelated cleanup.
4. **Keep changes local.** A routine parameter change should eventually require configuration, not editing C++ or recompiling.
5. **Separate responsibilities.** C++/Geant4 owns physics, geometry construction, source generation, tracking, and scoring; Python will own experiment definition, case orchestration, analysis, and artifacts; Docker owns dependencies and isolation; Geant4 macros own runtime commands and viewer setup.
6. **Report verification honestly.** Distinguish inspection, compilation, execution, numerical checks, and physical validation.
7. **Do not invent results.** Never claim a Monte Carlo output or physical validation result without actually executing and evaluating it.

## Current repository layout (reported; verify before use)

- `docker/Dockerfile` — container image
- `docker-compose.yaml` — container runtime/mount/display configuration
- `geant4-ws/LunarDevice/exampleB1.cc` — application entry point and physics/run setup
- `geant4-ws/LunarDevice/include/` and `src/` — geometry, source, actions, scoring
- `geant4-ws/LunarDevice/init_vis.mac` and `vis.mac` — interactive viewer initialization
- `geant4-ws/LunarDevice/macros/vis_wireframe.mac` and `vis_chip_only.mac` — display presets
- `geant4-ws/LunarDevice/run_lunar.mac` — baseline batch macro
- `geant4-ws/LunarDevice/build/` and `geant4-ws/B1-build/` — generated build output, not source

CSV logging, variable-thickness geometry, and Python analysis were proposed as an optional patch. **Do not assume the patch is installed or tested.** Verify its presence and output schema in the live checkout.

## Geant4 and physics invariants

- Document the active reference physics list, Geant4 version, relevant physics data, production cuts, and run configuration. Do not silently replace the physics list or tune cuts for more attractive results.
- Distinguish an incoming primary proton from a reaction product and from descendants of that product. A neutron created in aluminum is not necessarily a proton that simply 'turned into' a neutron.
- A particle history consists of events, tracks, steps, physical processes, and secondary particles. Preserve track IDs, parent IDs, creator processes, pre/post-step volumes, and units when implementing reaction logging.
- Keep event-level energy deposition distinct from step-level records, neutron yield, and boundary-crossing counts. Do not label an entry into the cavity as full enclosure transmission.
- Distinguish direct neutron-step energy deposition from neutron-descendant contributions. Do not claim ancestry-resolved Si energy deposition until ancestry tracking is implemented and tested.
- Do not interpret a fixed number of simulated primaries as lunar dose rate; a physically normalized spectrum, source area, angle, and time normalization are required.
- Do not equate absorbed dose with SEU/SEL/SEE probability. Device-sensitive volume and charge-collection/response models would be additional work.
- For comparisons, control and record random seeds, event counts, geometry, source distribution, physics settings, and sampling uncertainty. A matching seed does not automatically make different geometries identical random histories.
- For geometry changes, validate positive dimensions, containment, navigation, and overlap checks; do not assume CAD meshes preserve materials or are watertight.

## Configuration and experiment design

**Target architecture, not a claim of existing functionality:** a standalone YAML experiment definition is validated by Python, expanded into explicit cases, translated into supported Geant4 commands/inputs, and executed as one independent Geant4 process per case. Generated files and resolved settings are archived with each case.

- Reject unknown configuration keys, unsupported materials, nonphysical dimensions, conflicting settings, and unsupported physics options rather than silently ignoring them.
- Use explicit units (e.g., mm, MeV) and document all reference frames and angular conventions.
- Keep geometry, source, physics, scoring, visualization, and post-processing configurable independently where the code genuinely supports it.
- Prefer Geant4 UI commands via a messenger for simple geometry/material adjustments. Do not introduce runtime commands in documentation until their C++ implementation exists.
- Keep C++ fixed for ordinary case sweeps; code changes remain appropriate when adding genuinely new geometry primitives or physics/scoring capabilities.
- For complex STEP models, prefer an explicit conversion/geometry-loading stage (e.g., tessellated geometry or validated GDML) over generating a unique `DetectorConstruction.cc` for every part. This is a future design decision, not implemented functionality.
- Avoid extra dependencies, databases, schedulers, and dashboards until justified by a real experiment workflow.

## CSV and artifact contracts

For any CSV implementation, define its schema in the relevant documentation and test it. If the optional logging patch is present, inspect it before naming columns or promising outputs. Expected logical datasets include event summaries, proton inelastic reactions in aluminum, neutron birth records, and selected boundary crossings; exact fields must come from the code.

- Record run/event identity, worker identity if applicable, clear physical units, and process/material provenance.
- Never append results from different experimental configurations into an indistinguishable file.
- Multithreaded output must not have racing file writes. Per-worker files plus a deterministic merge are acceptable.
- Preserve zero-event cases in denominators and summary reporting; do not compute rates solely from rows with reactions.
- Avoid double-counting particles that cross a boundary repeatedly; define whether a metric means unique tracks, first crossings, or crossing occurrences.
- For future experiments, archive the resolved configuration, executable/source revision, Geant4/runtime versions, seed, process exit code, raw outputs, summary, and visualization settings.
- Treat captures as diagnostic illustrations, not proof of statistical frequency. Use small visualization runs, separate from high-statistics production runs.

## Visualization constraints

- The currently working interactive path uses TSG with an aluminum wireframe and a solid green silicon chip. Preserve it as a known-good baseline.
- Original B1 `Envelope`, `Shape1`, and `Shape2` settings do not correspond to the LunarDevice geometry; do not reintroduce those references.
- Avoid duplicate `/vis/open`, `/vis/drawVolume`, trajectory-model creation, and frequent `/vis/scene/notifyHandlers` calls across layered macros.
- Do not assume transparent surfaces work: prior TSG sessions crashed on redraw. Prefer wireframe until a driver-specific fix is verified.
- When coloring proton and neutron tracks separately, use a single particle-ID trajectory model rather than conflicting charge-based models.
- Treat `TSG_OFFSCREEN` screenshots as an optional capability that must be tested in the actual container, not an already verified feature.

## Development and verification

Before substantive changes:

1. Read the targeted source files, CMake rules, macros, and relevant README documentation.
2. Inspect `docker compose config --services` before choosing a service name; never guess it.
3. Inspect Geant4 discovery and the current physics-list selection before writing commands.
4. Check the working tree with `git status --short`; do not overwrite unrelated changes.

After changes, when the environment supports it:

1. Configure and build `geant4-ws/LunarDevice` with its existing CMake/Ninja setup.
2. Run a small **headless** batch-mode smoke test; avoid requiring GUI/X11 in automated tests.
3. Check for crashes, nonzero exit codes, geometry overlaps, missing required CSVs, invalid row counts, and nonfinite numbers.
4. Compare unchanged baseline metrics within the expected Monte Carlo variation; explain differences rather than demanding exact equality.
5. For new scoring, write focused checks of units, event aggregation, parent/child relationships, and MT merging.
6. If visualization changes, test viewer setup separately; do not substitute GUI success for physics validation.

Do not claim a check passed unless it was run. If Docker or Geant4 is unavailable, state the blocked check and supply the exact command to run locally.

## Repository / Git behavior

- Do not commit or push unless explicitly requested.
- Do not reset, delete, or overwrite unrelated work.
- Keep generated build products, massive CSVs, screenshots, and temporary artifacts out of Git by default; keep source, macros, small reference inputs, and experiment definitions versioned.
- Avoid machine-specific absolute paths and credentials.
- Prefer a writable non-root container user; do not work around permissions with privileged containers or `chmod 777`.
- At completion, report what changed, why, which tests ran, outstanding risks, and commands the maintainer should execute.

## Default decision criterion

**What is the smallest reliable change that provides the requested physical or experimental capability while preserving the working baseline and keeping the next change cheap?**
