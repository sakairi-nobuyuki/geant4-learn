# Geant4 Docker Learning Environment

![Geant4](https://img.shields.io/badge/Geant4-Monte%20Carlo-blue)
![Docker](https://img.shields.io/badge/Docker-Enabled-2496ED?logo=docker&logoColor=white)
![Ubuntu](https://img.shields.io/badge/Ubuntu-Linux-E95420?logo=ubuntu&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-Ninja-064F8C?logo=cmake&logoColor=white)
![Status](https://img.shields.io/badge/Status-Educational-green)

A Docker-based educational environment for learning Geant4 through Monte Carlo particle transport simulations.

This repository provides a reproducible Linux-based development environment and a modified Geant4 Basic Example B1 for studying particle interactions with a simplified semiconductor device enclosed in an aluminum housing.

The project is intended for education, experimentation, and prototyping rather than validated radiation qualification.

## 1. Overview

Geant4 is a toolkit for simulating the passage of particles through matter.

It supports:

- Particle transport through three-dimensional geometries
- Electromagnetic interactions
- Hadronic and nuclear interactions
- Secondary particle generation
- Energy deposition and absorbed dose calculations
- Particle trajectory visualization
- Monte Carlo simulations with multiple threads

This repository uses Geant4 to investigate how energetic particles interact with aluminum shielding and silicon semiconductor devices.

The initial simulation is inspired by radiation environments relevant to lunar surface electronics.

However, the current implementation uses a simplified monoenergetic proton source and does not yet reproduce the complete lunar radiation environment.

## 2. Repository Structure

```text
geant-learn/
├── README.md
├── docker/
│   └── Dockerfile
├── docker-compose.yaml
└── geant4-ws/
    ├── B1-build/
    │   └── ...
    └── LunarDevice/
        ├── CMakeLists.txt
        ├── GNUmakefile
        ├── README.md
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
        ├── macros/
        │   ├── vis_chip_only.mac
        │   └── vis_wireframe.mac
        ├── init_vis.mac
        ├── vis.mac
        ├── vis_lunar.mac
        ├── run_lunar.mac
        └── build/
```

The build directories contain generated CMake files, compiled binaries, and other temporary files.

These should normally be excluded from Git.

## 3. Requirements

Recommended environment:

- Ubuntu Linux or Ubuntu on WSL2
- Docker Engine
- Docker Compose

The Docker image is intended to provide the Geant4 development environment.

Graphical visualization additionally requires access to a display server.

On WSL2, WSLg can provide graphical application support.

## 4. Docker Environment

### Build and start the container

From the repository root:

```bash
cd ~/projects/geant-learn

docker compose build
docker compose up -d
```

Check the available services:

```bash
docker compose config --services
```

Enter the running container:

```bash
docker compose exec <service-name> bash
```

Replace `<service-name>` with the service defined in `docker-compose.yaml`.

Alternatively, list the containers:

```bash
docker compose ps
```

The exact container user, mounted paths, and Geant4 installation location depend on the Docker configuration.

### Verify Geant4

Inside the container:

```bash
geant4-config --version
```

Locate the CMake configuration if necessary:

```bash
find /usr/local /opt "$HOME" \
  -name Geant4Config.cmake 2>/dev/null
```

If Geant4 is installed but not automatically detected, pass the directory containing `Geant4Config.cmake` through `Geant4_DIR`.

## 5. Building LunarDevice

Navigate to the application source directory:

```bash
cd ~/geant4-ws/LunarDevice
```

Create a separate build directory:

```bash
mkdir -p build
cd build
```

Configure the project:

```bash
cmake -G Ninja ..
```

If Geant4 cannot be found:

```bash
cmake -G Ninja \
  -DGeant4_DIR=/path/to/Geant4Config-directory \
  ..
```

Compile:

```bash
cmake --build . -j 4
```

The resulting executable is:

```text
build/exampleB1
```

The executable name is inherited from the original Geant4 B1 example.

## 6. Simulation Geometry

The initial LunarDevice simulation contains three main components:

| Component | Material | Dimensions |
|---|---|---|
| World | Vacuum | 600 × 600 × 600 mm |
| Aluminum enclosure | Aluminum | 100 × 100 × 30 mm |
| Inner cavity | Vacuum | 96 × 96 × 26 mm |
| Semiconductor chip | Silicon | 10 × 10 × 0.5 mm |

The aluminum enclosure has a nominal wall thickness of 2 mm.

The silicon chip is positioned at the center of the internal cavity.

The geometry is implemented using:

- `G4Box`
- `G4LogicalVolume`
- `G4PVPlacement`
- `G4NistManager`

The silicon logical volume is designated as the scoring volume for energy deposition.

### Geometry hierarchy

```text
World (vacuum)
└── AlCase (aluminum)
    └── Cavity (vacuum)
        └── Chip (silicon)
```

The model is intentionally simple to make the physical processes and software structure easier to understand.

## 7. Radiation Source

The initial simulation uses a monoenergetic proton source.

| Parameter | Initial value |
|---|---|
| Particle | Proton |
| Energy | 100 MeV |
| Direction | Negative Z |
| Initial Z position | +100 mm |
| Source area | 10 × 10 mm |
| Typical event count | 10,000 |

The proton source is implemented using `G4ParticleGun` in `PrimaryGeneratorAction.cc`.

The transverse source position is sampled uniformly across a square area corresponding to the projected chip dimensions.

This source is designed for initial testing.

It does not yet represent an isotropic cosmic-ray environment or a physically normalized lunar radiation spectrum.

## 8. Physics Models

Geant4 supports electromagnetic and hadronic processes relevant to this simulation.

Important examples include:

- Ionization energy loss
- Multiple Coulomb scattering
- Hadronic elastic scattering
- Hadronic inelastic reactions
- Secondary electron production
- Secondary neutron production
- Gamma-ray generation and transport

The Geant4 B1 example normally uses the `QBBC` reference physics list.

The active physics list is selected in `exampleB1.cc` and should be checked when comparing simulation results.

Alternative physics lists, such as `FTFP_BERT`, may be considered for further studies.

The selection of a physics list is part of the physical modeling assumptions and must be documented when reporting results.

## 9. Running the Simulation

### Batch mode

From the build directory:

```bash
cd ~/geant4-ws/LunarDevice/build

./exampleB1 ../run_lunar.mac
```

The default educational run generates 10,000 primary protons.

The output includes:

- Number of simulated events
- Total deposited energy in silicon
- Mass of the scoring volume
- Absorbed dose
- Statistical information

Example output:

```text
Run Summary
  Number of events processed : 10000

cumulated edep per run in scoring volume = ...
mass of scoring volume = 116.5 mg

Absorbed dose per run in scoring volume = ...
```

The absorbed dose is calculated as:

$$
D = \frac{E_{\mathrm{dep}}}{m}
$$

where:

- $D$ is absorbed dose in Gy
- $E_{\mathrm{dep}}$ is deposited energy in J
- $m$ is scoring volume mass in kg

The result represents the dose from the simulated primary events, not a dose rate per hour or per year.

Converting simulation results into physically meaningful dose rates requires a properly normalized incident radiation flux.

## 10. Visualization

Geant4 provides several visualization systems.

This project has been tested interactively using a Qt-based viewer with the TSG graphics driver.

Visualization is useful for:

- Inspecting the geometry
- Confirming the particle source direction
- Observing particle trajectories
- Examining scattering
- Identifying secondary particle tracks
- Checking the relative positions of the enclosure and silicon chip

### 10.1 Starting the viewer

From the application build directory:

```bash
cd ~/geant4-ws/LunarDevice/build

./exampleB1
```

When the executable starts in interactive mode, the B1 application normally executes `init_vis.mac`, which loads `vis.mac`.

Keep the visualization macro in the source directory and rebuild or copy it to the build directory after editing.

For manual macro execution in the Qt command interface:

```text
/control/execute ../macros/vis_wireframe.mac
```

Use a fresh viewer/session when a macro contains `/vis/open` or creates new scenes and trajectory models.

### 10.2 Basic visualization commands

The following commands are useful in an initialized Geant4 session:

```text
/vis/open TSG
/vis/drawVolume
/vis/viewer/set/style wireframe
/vis/viewer/set/viewpointThetaPhi 65 35
/vis/viewer/refresh
```

These commands should not be repeatedly used to create additional viewers if a viewer is already open.

### 10.3 Displaying the silicon chip

The semiconductor is much smaller than the enclosure.

To inspect the chip, hide the surrounding volumes:

```text
/vis/viewer/set/autoRefresh false

/vis/geometry/set/visibility World 0 false
/vis/geometry/set/visibility AlCase 0 false
/vis/geometry/set/visibility Cavity 0 false

/vis/geometry/set/visibility Chip 0 true
/vis/geometry/set/colour Chip 0 0 1 0 1

/vis/viewer/set/style surface
/vis/viewer/zoom 5

/vis/viewer/refresh
```

The chip is displayed in green.

If the geometry does not update, issue:

```text
/vis/scene/notifyHandlers
```

Use this command sparingly, particularly with graphics drivers that have exhibited stability issues.

### 10.4 Displaying the aluminum enclosure

A wireframe enclosure is useful because the semiconductor is located inside it.

```text
/vis/viewer/set/autoRefresh false

/vis/geometry/set/visibility AlCase 0 true
/vis/geometry/set/forceWireframe AlCase 0 true
/vis/geometry/set/colour AlCase 0 0.7 0.7 0.7 1

/vis/geometry/set/visibility Chip 0 true
/vis/geometry/set/colour Chip 0 0 1 0 1

/vis/viewer/refresh
```

The recommended default visualization uses:

- Gray wireframe for aluminum
- Green solid geometry for silicon
- Colored particle trajectories

This configuration avoids relying on transparency rendering.

### 10.5 Transparent materials

Geant4 supports alpha values in visualization color commands.

For example:

```text
/vis/geometry/set/colour AlCase 0 0.7 0.7 0.7 0.15
```

The final parameter controls opacity.

However, transparency rendering may depend on the selected graphics driver and graphical environment.

In the current Docker-based setup, crashes have been observed during visualization updates after changing geometry attributes.

For this reason, wireframe rendering is recommended until transparency behavior has been verified.

### 10.6 Displaying particle trajectories

Particle trajectories can be accumulated and displayed.

A visualization macro can contain:

```text
/vis/scene/add/trajectories smooth
/vis/scene/endOfEventAction accumulate
```

To distinguish particle species, use:

```text
/vis/modeling/trajectories/create/drawByParticleID

/vis/modeling/trajectories/drawByParticleID-0/set proton cyan
/vis/modeling/trajectories/drawByParticleID-0/set neutron yellow
/vis/modeling/trajectories/drawByParticleID-0/set e- red
/vis/modeling/trajectories/drawByParticleID-0/set gamma green
```

Only one trajectory drawing model should be selected when interpreting particle colors.

To generate a small number of visible tracks:

```text
/run/beamOn 20
```

Avoid visualizing thousands of events simultaneously when inspecting individual particle interactions.

### 10.7 Camera controls

Change the viewpoint:

```text
/vis/viewer/set/viewpointThetaPhi 65 35
```

Zoom in:

```text
/vis/viewer/zoom 3
```

Update the viewer:

```text
/vis/viewer/refresh
```

For a fresh visualization session, restart the application rather than repeatedly creating scenes, trajectory models, and viewers.

## 11. Investigating Particle Interactions

Geant4 can simulate nuclear reactions and secondary particle production when the appropriate physics processes are enabled.

For example, an energetic proton may undergo an inelastic interaction with an aluminum nucleus and produce secondary particles, including neutrons.

### Step-by-step tracking

Enable tracking verbosity:

```text
/tracking/verbose 2
/run/beamOn 1
```

This displays detailed information about particle transport steps.

Processes of interest include:

| Process | Description |
|---|---|
| hIoni | Hadron ionization |
| msc | Multiple scattering |
| hadElastic | Hadronic elastic scattering |
| protonInelastic | Proton-induced inelastic interaction |
| Transportation | Particle transportation |

The actual processes depend on the selected physics list and simulated interactions.

### Secondary particles

To study secondary neutron generation, future extensions should record:

- Primary particle type and energy
- Interaction position
- Material and logical volume
- Interaction process
- Secondary particle species
- Secondary kinetic energy
- Particle direction
- Energy deposition in silicon

The following Geant4 interfaces are particularly useful:

```cpp
G4Track::GetDefinition()
G4Track::GetCreatorProcess()
G4Step::GetPostStepPoint()
G4Step::GetSecondaryInCurrentStep()
```

A CSV-based particle interaction logger is planned.

## 12. Troubleshooting

### Geant4Config.cmake not found

Error:

```text
Could not find Geant4Config.cmake
```

Locate the configuration:

```bash
find / -name Geant4Config.cmake 2>/dev/null
```

Pass the containing directory to CMake:

```bash
cmake -G Ninja \
  -DGeant4_DIR=/path/to/config-directory \
  ..
```

### Qt cannot connect to display

Error:

```text
qt.qpa.xcb: could not connect to display
```

Check the container display environment:

```bash
echo $DISPLAY
ls -l /tmp/.X11-unix/
```

Ensure Docker Compose forwards the display socket and provides the required X11 authorization.

On WSL2, WSLg and Docker integration require appropriate display forwarding.

### Logical volume not found

Error:

```text
Logical volume "Envelope" not found
```

The original Geant4 B1 visualization macro refers to geometry that is no longer present.

The LunarDevice geometry uses:

```text
World
AlCase
Cavity
Chip
```

Remove old visualization commands referring to:

```text
Envelope
Shape1
Shape2
```

### Segmentation fault during visualization

If the viewer crashes after changing geometry attributes:

1. Restart Geant4.
2. Use a minimal visualization macro.
3. Avoid transparent surfaces initially.
4. Use wireframe rendering.
5. Avoid unnecessary scene rebuilds.
6. Try another available visualization driver.
7. Collect a GDB backtrace if the issue persists.

To debug:

```bash
gdb --args ./exampleB1
```

Inside GDB:

```text
run
thread apply all bt 15
```

### Simulation runs but no window appears

Geant4 supports batch execution without a graphical interface.

For example:

```bash
./exampleB1 ../run_lunar.mac
```

This computes the physics results but does not necessarily open an interactive visualization window.

For interactive visualization, start the executable without the batch macro, using a working display server.

## 13. Current Status

Implemented or demonstrated:

- [x] Docker-based Geant4 environment
- [x] Basic Example B1 compilation
- [x] Aluminum enclosure geometry
- [x] Silicon chip geometry
- [x] Monoenergetic proton irradiation
- [x] Silicon energy deposition scoring
- [x] Absorbed dose calculation
- [x] Interactive particle trajectory visualization
- [x] Aluminum wireframe visualization

Planned development:

- [ ] Per-event energy deposition output
- [ ] CSV-based interaction logging
- [ ] Secondary neutron production analysis
- [ ] Proton energy sweeps
- [ ] Aluminum shielding thickness studies
- [ ] Energy deposition histograms
- [ ] Lunar GCR and SEP spectra
- [ ] Lunar regolith secondary particle modeling
- [ ] Semiconductor SEE response modeling
- [ ] Validation and uncertainty quantification

## 14. Limitations

The present model is intended for learning and preliminary investigation.

Important limitations include:

1. The radiation source is monoenergetic.
2. A realistic lunar particle energy spectrum is not yet implemented.
3. The angular distribution is simplified.
4. The lunar regolith is not included.
5. The semiconductor is represented by homogeneous silicon.
6. Detailed semiconductor structures and charge collection are not modeled.
7. Absorbed dose does not directly determine SEU or SEL probability.
8. Quantitative radiation predictions require validation against experimental or reference data.

Simulation results should not be interpreted as device qualification evidence without further verification and validation.

## 15. Future Applications

The repository may serve as an introductory platform for:

- Space radiation transport simulations
- Semiconductor radiation response studies
- Radiation shielding optimization
- Secondary neutron generation studies
- Lunar surface electronics investigations
- Monte Carlo uncertainty analysis
- Experimental validation workflows

The long-term objective is to connect particle transport simulations with device-level response models and engineering design decisions.

## Codex

```shell
cd ~/geant4-ws

codex --sandbox danger-full-access \
      --ask-for-approval on-request
```


## 16. References

- [Geant4 Official Website](https://geant4.web.cern.ch/)
- [Geant4 Documentation](https://geant4.web.cern.ch/documentation/)
- [Geant4 GitHub Repository](https://github.com/Geant4/geant4)
- [Geant4 Basic Examples](https://geant4.web.cern.ch/documentation/dev/bfad_html/ForApplicationDevelopers/Examples/BasicCodes.html)
- [Geant4 Visualization Documentation](https://geant4.web.cern.ch/documentation/dev/bfad_html/ForApplicationDevelopers/Visualization/enhanceddrawing.html)

## 17. Disclaimer

This repository is provided for educational and experimental purposes.

It is not a validated space radiation analysis tool and should not be used as the sole basis for safety-critical or mission-critical engineering decisions.

The simulation models, physics assumptions, and numerical results are subject to further verification and validation.



