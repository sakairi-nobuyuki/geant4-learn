#include "LunarRecorder.hh"

#include "G4Event.hh"
#include "G4ParticleDefinition.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"
#include "G4ios.hh"

#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <stdexcept>
#include <string>

namespace B1 {
namespace {
  std::string Name(const G4VPhysicalVolume* volume) {
    return volume ? std::string(volume->GetName()) : "OUTSIDE";
  }
  std::string ProcessName(const G4VProcess* process) {
    return process ? std::string(process->GetProcessName()) : "none";
  }
  void Open(std::ofstream& file, const std::filesystem::path& path,
            const std::string& header) {
    file.open(path, std::ios::out | std::ios::trunc);
    if (!file) throw std::runtime_error("Cannot write CSV: " + path.string());
    file << std::setprecision(10) << header << '\n';
  }
}

void LunarRecorder::OpenFiles(G4int runId) {
  fEvents.close(); fReactions.close(); fNeutrons.close(); fCrossings.close();
  const char* env = std::getenv("LUNAR_OUTPUT_DIR");
  const std::filesystem::path folder = env && *env ? env : "results";
  std::filesystem::create_directories(folder);
  const auto suffix = "_r" + std::to_string(runId) + "_t" +
                      std::to_string(G4Threading::G4GetThreadId()) + ".csv";
  Open(fEvents, folder / ("events" + suffix),
       "run_id,event_id,primary_entered_Al,primary_entered_cavity,primary_energy_at_cavity_MeV,"
       "proton_inelastic_in_Al,neutrons_created_in_Al,neutrons_entered_Si,"
       "Si_edep_MeV,primary_proton_Si_edep_MeV,direct_neutron_Si_edep_MeV");
  Open(fReactions, folder / ("reactions" + suffix),
       "run_id,event_id,track_id,parent_id,particle,process,volume,"
       "pre_energy_MeV,post_energy_MeV,x_mm,y_mm,z_mm,secondaries_total,secondary_neutrons");
  Open(fNeutrons, folder / ("neutrons" + suffix),
       "run_id,event_id,parent_track_id,creator_process,"
       "energy_MeV,x_mm,y_mm,z_mm,dx,dy,dz");
  Open(fCrossings, folder / ("crossings" + suffix),
       "run_id,event_id,track_id,parent_id,particle,from_volume,to_volume,"
       "energy_MeV,x_mm,y_mm,z_mm");
  fRunId = runId;
}

void LunarRecorder::BeginEvent(const G4Event* event) {
  const auto* run = G4RunManager::GetRunManager()->GetCurrentRun();
  const auto runId = run ? run->GetRunID() : 0;
  if (runId != fRunId) OpenFiles(runId);
  fEventId = event->GetEventID();
  fProtonInelastic = fNeutronsCreated = fNeutronsIntoChip = 0;
  fPrimaryEnteredCase = fPrimaryEnteredCavity = 0;
  fPrimaryEnergyAtCavity = -1.;
  fSiliconEdep = fPrimarySiliconEdep = fNeutronSiliconEdep = 0.;
}

void LunarRecorder::ProcessStep(const G4Step* step) {
  const auto* track = step->GetTrack();
  const auto* pre = step->GetPreStepPoint();
  const auto* post = step->GetPostStepPoint();
  if (!pre || !post) return;
  const auto preName = Name(pre->GetPhysicalVolume());
  const auto postName = Name(post->GetPhysicalVolume());
  const auto particle = std::string(track->GetDefinition()->GetParticleName());
  const auto process = ProcessName(post->GetProcessDefinedStep());
  const bool primaryProton = track->GetParentID() == 0 && particle == "proton";

  // All energy deposited in Si, irrespective of the depositing particle.
  if (preName == "Chip") {
    const auto edep = step->GetTotalEnergyDeposit() / MeV;
    fSiliconEdep += edep;
    if (primaryProton) fPrimarySiliconEdep += edep;
    if (particle == "neutron") fNeutronSiliconEdep += edep;
  }

  // A boundary crossing is identified by the post-step fGeomBoundary status.
  // Record primary crossing both sides of the Al wall into the cavity.
  if (post->GetStepStatus() == fGeomBoundary && preName != postName) {
    const bool enteringCase = primaryProton && preName == "World" && postName == "AlCase";
    const bool enteringCavity = primaryProton && postName == "Cavity" &&
                                (preName == "AlCase" || preName == "World");
    const bool enteringChip = postName == "Chip";
    if (enteringCase) fPrimaryEnteredCase = 1;
    if (enteringCavity) {
      fPrimaryEnteredCavity = 1;
      fPrimaryEnergyAtCavity = post->GetKineticEnergy() / MeV;
    }
    if (enteringChip && particle == "neutron") ++fNeutronsIntoChip;
    if (enteringCase || enteringCavity || enteringChip) {
      const auto pos = post->GetPosition();
      fCrossings << fRunId << ',' << fEventId << ',' << track->GetTrackID() << ','
        << track->GetParentID() << ',' << particle << ',' << preName << ',' << postName << ','
        << post->GetKineticEnergy()/MeV << ',' << pos.x()/mm << ',' << pos.y()/mm
        << ',' << pos.z()/mm << '\n';
    }
  }

  const auto* secondaries = step->GetSecondaryInCurrentStep();
  G4int neutronCount = 0;
  if (preName == "AlCase" && secondaries) {
    for (const auto* sec : *secondaries) {
      if (sec->GetDefinition()->GetParticleName() != "neutron") continue;
      ++neutronCount;
      ++fNeutronsCreated;
      const auto pos = sec->GetPosition();
      const auto dir = sec->GetMomentumDirection();
      fNeutrons << fRunId << ',' << fEventId << ',' << track->GetTrackID() << ','
        << process << ',' << sec->GetKineticEnergy()/MeV
        << ',' << pos.x()/mm << ',' << pos.y()/mm << ',' << pos.z()/mm
        << ',' << dir.x() << ',' << dir.y() << ',' << dir.z() << '\n';
    }
  }

  if (preName == "AlCase" && particle == "proton" && process == "protonInelastic") {
    ++fProtonInelastic;
    const auto pos = post->GetPosition();
    fReactions << fRunId << ',' << fEventId << ',' << track->GetTrackID() << ','
      << track->GetParentID() << ',' << particle << ',' << process << ',' << preName
      << ',' << pre->GetKineticEnergy()/MeV << ',' << post->GetKineticEnergy()/MeV
      << ',' << pos.x()/mm << ',' << pos.y()/mm << ',' << pos.z()/mm
      << ',' << (secondaries ? secondaries->size() : 0) << ',' << neutronCount << '\n';
  }
}

void LunarRecorder::EndEvent(const G4Event*) {
  fEvents << fRunId << ',' << fEventId << ',' << fPrimaryEnteredCase << ','
    << fPrimaryEnteredCavity << ',' << fPrimaryEnergyAtCavity << ','
    << fProtonInelastic << ',' << fNeutronsCreated << ',' << fNeutronsIntoChip << ','
    << fSiliconEdep << ',' << fPrimarySiliconEdep << ',' << fNeutronSiliconEdep << '\n';
}

} // namespace B1
