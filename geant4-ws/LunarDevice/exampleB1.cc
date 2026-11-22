//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file exampleB1.cc
/// \brief Main program of the basic/B1 example

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "LunarRecorder.hh"
#include "ParticleEnergyVisualization.hh"
#include "QBBC.hh"

#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
// #include "Randomize.hh"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>

using namespace B1;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

namespace {
namespace fs = std::filesystem;

constexpr auto usage =
  "Usage: lunar_device [--target [NAME] | --target=NAME] "
  "[--vis-chip transparent|wireframe] "
  "[--vis-mode particle_type|particle_energy|charge | --vis-macro FILE | --macro FILE | FILE]";

struct Options {
  // No value selects the environment or default. An empty value generates a name.
  std::optional<std::string> target;
  std::optional<std::string> macro;
  std::optional<std::string> visMacro;
  std::optional<std::string> visMode;
  std::optional<std::string> visChip;
};

Options ParseOptions(int argc, char** argv)
{
  Options options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--target" || arg.compare(0, 9, "--target=") == 0) {
      if (options.target) throw std::runtime_error("--target may only be specified once");
      if (arg != "--target") {
        options.target = arg.substr(9);
        if (options.target->empty()) throw std::runtime_error("--target= requires a name");
      } else {
        options.target = "";
        if (i + 1 < argc) {
          const std::string next = argv[i + 1];
          if (!next.empty() && next.front() != '-' && fs::path(next).extension() != ".mac") {
            options.target = argv[++i];
          }
        }
      }
      continue;
    }

    if (arg == "--vis-mode") {
      if (++i == argc || options.visMode) {
        throw std::runtime_error("--vis-mode requires one mode");
      }
      const std::string mode = argv[i];
      if (mode != "particle_type" && mode != "particle_energy" && mode != "charge") {
        throw std::runtime_error("Unknown visualization mode " + mode);
      }
      options.visMode = mode;
      continue;
    }

    if (arg == "--vis-chip") {
      if (++i == argc || options.visChip) {
        throw std::runtime_error("--vis-chip requires one appearance");
      }
      const std::string appearance = argv[i];
      if (appearance != "transparent" && appearance != "wireframe") {
        throw std::runtime_error("Unknown chip appearance " + appearance);
      }
      options.visChip = appearance;
      continue;
    }

    if (arg == "--vis-macro") {
      if (++i == argc || options.visMacro || std::string(argv[i]).empty() || argv[i][0] == '-') {
        throw std::runtime_error("--vis-macro requires one startup macro filename");
      }
      options.visMacro = argv[i];
      continue;
    }

    std::string macro = arg;
    if (arg == "--macro") {
      if (++i == argc) throw std::runtime_error("--macro requires a filename");
      macro = argv[i];
    } else if (!arg.empty() && arg.front() == '-') {
      throw std::runtime_error("Unknown option " + arg);
    }
    if (macro.empty() || macro.front() == '-' || options.macro) {
      throw std::runtime_error("Specify one macro filename");
    }
    options.macro = macro;
  }
  if (options.macro && options.visMacro) {
    throw std::runtime_error("Choose either an interactive startup macro or a batch macro");
  }
  if (options.visMode && (options.macro || options.visMacro)) {
    throw std::runtime_error("--vis-mode selects the built-in interactive startup");
  }
  if (options.visChip && (options.macro || options.visMacro)) {
    throw std::runtime_error("--vis-chip selects an appearance for the built-in interactive startup");
  }
  return options;
}

void ConfigureOutput(const Options& options)
{
  const fs::path results = fs::path(LUNAR_PROJECT_ROOT) / "results";
  const auto defaultPath = results / "res";
  const char* env = std::getenv("LUNAR_OUTPUT_DIR");
  if (!options.target && env && *env) {
    const auto path = fs::weakly_canonical(fs::absolute(env));
    if (path == fs::weakly_canonical(defaultPath)) {
      throw std::runtime_error("LUNAR_OUTPUT_DIR cannot select the disposable results/res directory");
    }
    LunarRecorder::ConfigureOutput(path, false);
    return;
  }

  if (fs::is_symlink(results)) {
    throw std::runtime_error("Results root must be a directory, without a symbolic link: " +
                             results.string());
  }
  if (!options.target) {
    LunarRecorder::ConfigureOutput(defaultPath, true);
  } else if (options.target->empty()) {
    fs::create_directories(results);
    auto pattern = (results / "run-XXXXXX").string();
    const char* directory = mkdtemp(pattern.data());
    if (!directory) throw std::runtime_error("Cannot create a unique directory in " + results.string());
    LunarRecorder::ConfigureOutput(directory, false);
  } else {
    static const std::regex validName("[A-Za-z0-9][A-Za-z0-9_.-]*");
    if (*options.target == "res" || !std::regex_match(*options.target, validName)) {
      throw std::runtime_error("Invalid target name " + *options.target +
                               " (use letters, digits, dots, underscores or hyphens; res is reserved)");
    }
    LunarRecorder::ConfigureOutput(results / *options.target, false);
  }
}
}  // namespace

int main(int argc, char** argv)
{
  if (argc == 2 && std::string(argv[1]) == "--help") {
    std::cout << usage << '\n'
              << "A bare --target generates a directory when followed by an option, a .mac file,"
                 " or the end of the command line. Use --target=NAME for a name ending in .mac.\n";
    return 0;
  }

  Options options;
  try {
    options = ParseOptions(argc, argv);
    if (options.macro && !fs::is_regular_file(*options.macro)) {
      throw std::runtime_error("Cannot open macro " + *options.macro);
    }
    if (options.visMacro && !fs::is_regular_file(*options.visMacro)) {
      throw std::runtime_error("Cannot open macro " + *options.visMacro);
    }
    ConfigureOutput(options);
  } catch (const std::exception& error) {
    std::cerr << "lunar_device: " << error.what() << '\n' << usage << '\n';
    return 1;
  }

  // Keep LunarDevice options out of Geant4's UI and visualization arguments.
  int geantArgc = 1;
  char* geantArgv[] = {argv[0], nullptr};
  auto* ui = options.macro ? nullptr : new G4UIExecutive(geantArgc, geantArgv);

  // Optionally: choose a different Random engine...
  // G4Random::setTheEngine(new CLHEP::MTwistEngine);

  // use G4SteppingVerboseWithUnits
  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);

  // Construct the default run manager
  //
  auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // Set mandatory initialization classes
  //
  // Detector construction
  runManager->SetUserInitialization(new DetectorConstruction());

  // Physics list
  auto physicsList = new QBBC;
  physicsList->SetVerboseLevel(1);
  runManager->SetUserInitialization(physicsList);

  // Initialize visualization with the default graphics system
  auto visManager = new G4VisExecutive(geantArgc, geantArgv);
  // Constructors can also take optional arguments:
  // - a graphics system of choice, eg. "OGL"
  // - and a verbosity argument - see /vis/verbose guidance.
  // auto visManager = new G4VisExecutive(argc, argv, "OGL", "Quiet");
  // auto visManager = new G4VisExecutive("Quiet");
  visManager->Initialize();
  ParticleEnergyVisualization visualization(*visManager);
  runManager->SetUserInitialization(new ActionInitialization(visualization));

  // Get the pointer to the User Interface manager
  auto UImanager = G4UImanager::GetUIpointer();

  // Process macro or start UI session
  //
  if (options.macro) {
    UImanager->ExecuteMacroFile(options.macro->c_str());
  }
  else {
    UImanager->ApplyCommand("/control/alias lunarVisMode " +
                            options.visMode.value_or("particle_type"));
    UImanager->ApplyCommand("/control/alias lunarVisChip " +
                            options.visChip.value_or("transparent"));
    UImanager->ExecuteMacroFile(options.visMacro.value_or("init_vis.mac").c_str());
    if (UImanager->GetLastReturnCode() == 0) ui->SessionStart();
    delete ui;
  }
  const int status = UImanager->GetLastReturnCode();

  // Job termination
  // Free the store: user actions, physics_list and detector_description are
  // owned and deleted by the run manager, so they should not be deleted
  // in the main() program !

  delete visManager;
  delete runManager;
  return status == 0 ? 0 : 1;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo.....
