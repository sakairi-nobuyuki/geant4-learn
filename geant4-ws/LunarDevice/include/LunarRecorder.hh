#ifndef LunarRecorder_h
#define LunarRecorder_h

#include "G4ThreeVector.hh"
#include "globals.hh"
#include <fstream>

class G4Event;
class G4Step;

namespace B1 {

// One instance per Geant4 worker. No shared CSV file is written by two threads.
class LunarRecorder {
public:
  void BeginEvent(const G4Event* event);
  void ProcessStep(const G4Step* step);
  void EndEvent(const G4Event* event);

private:
  void OpenFiles(G4int runId);
  G4int fRunId = -1;
  G4int fEventId = -1;
  G4int fProtonInelastic = 0;
  G4int fNeutronsCreated = 0;
  G4int fNeutronsIntoChip = 0;
  G4int fPrimaryEnteredCase = 0;
  G4int fPrimaryEnteredCavity = 0;
  G4double fPrimaryEnergyAtCavity = -1.;
  G4double fSiliconEdep = 0.;
  G4double fPrimarySiliconEdep = 0.;
  G4double fNeutronSiliconEdep = 0.; // Direct neutron-step deposition only
  std::ofstream fEvents;
  std::ofstream fReactions;
  std::ofstream fNeutrons;
  std::ofstream fCrossings;
};

} // namespace B1
#endif
