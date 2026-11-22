#include "ParticleEnergyVisualization.hh"

#include "G4GenericMessenger.hh"
#include "G4ParticleDefinition.hh"
#include "G4Polyline.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4TrackingManager.hh"
#include "G4VTrajectoryModel.hh"
#include "G4VTrajectoryPoint.hh"
#include "G4VisAttributes.hh"
#include "G4VisManager.hh"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <vector>

namespace B1 {
namespace {

constexpr auto missingEnergy = std::numeric_limits<G4double>::quiet_NaN();

class EnergyPoint final : public G4VTrajectoryPoint {
 public:
  EnergyPoint(const G4ThreeVector& position, G4double incomingEnergy)
    : fPosition(position), fIncomingEnergy(incomingEnergy) {}

  const G4ThreeVector GetPosition() const override { return fPosition; }
  G4double IncomingEnergy() const { return fIncomingEnergy; }

 private:
  G4ThreeVector fPosition;
  G4double fIncomingEnergy;
};

class EnergyTrajectory final : public G4VTrajectory {
 public:
  explicit EnergyTrajectory(const G4Track& track)
    : fParticleName(track.GetDefinition()->GetParticleName()),
      fTrackID(track.GetTrackID()),
      fParentID(track.GetParentID()),
      fCharge(track.GetDynamicParticle()->GetCharge()),
      fPDGEncoding(track.GetDefinition()->GetPDGEncoding()),
      fInitialMomentum(track.GetMomentum())
  {
    fPoints.emplace_back(track.GetPosition(), missingEnergy);
  }

  G4int GetTrackID() const override { return fTrackID; }
  G4int GetParentID() const override { return fParentID; }
  G4String GetParticleName() const override { return fParticleName; }
  G4double GetCharge() const override { return fCharge; }
  G4int GetPDGEncoding() const override { return fPDGEncoding; }
  G4ThreeVector GetInitialMomentum() const override { return fInitialMomentum; }
  G4int GetPointEntries() const override { return static_cast<G4int>(fPoints.size()); }
  G4VTrajectoryPoint* GetPoint(G4int i) const override
  {
    return const_cast<EnergyPoint*>(&fPoints[i]);
  }

  G4double SegmentEnergy(G4int endpoint) const { return fPoints[endpoint].IncomingEnergy(); }

  void AppendStep(const G4Step* step) override
  {
    const auto* pre = step->GetPreStepPoint();
    fPoints.emplace_back(step->GetPostStepPoint()->GetPosition(),
                         pre ? pre->GetKineticEnergy() : missingEnergy);
  }

  void MergeTrajectory(G4VTrajectory* trajectory) override
  {
    // Geant4 can suspend and resume a track. The new initial point is the
    // previous endpoint, so keep only the new segment endpoints.
    auto* resumed = dynamic_cast<EnergyTrajectory*>(trajectory);
    if (!resumed) return;
    fPoints.insert(fPoints.end(), std::make_move_iterator(std::next(resumed->fPoints.begin())),
                   std::make_move_iterator(resumed->fPoints.end()));
  }

  G4VTrajectory* CloneForMaster() const override { return new EnergyTrajectory(*this); }

 private:
  G4String fParticleName;
  G4int fTrackID;
  G4int fParentID;
  G4double fCharge;
  G4int fPDGEncoding;
  G4ThreeVector fInitialMomentum;
  std::vector<EnergyPoint> fPoints;
};

G4Colour BaseColour(const G4String& particle)
{
  if (particle == "proton") return {0.10, 0.85, 1.00};
  if (particle == "neutron") return {1.00, 1.00, 0.00};
  if (particle == "gamma") return {0.00, 1.00, 0.00};
  if (particle == "e-") return {1.00, 0.12, 0.12};
  if (particle == "e+") return {1.00, 0.00, 1.00};
  return {0.80, 0.80, 0.80};
}

G4Colour ScaleColour(const G4Colour& hue, G4double brightness)
{
  return {hue.GetRed() * brightness, hue.GetGreen() * brightness,
          hue.GetBlue() * brightness, 1.0};
}

class ParticleEnergyModel final : public G4VTrajectoryModel {
 public:
  ParticleEnergyModel(G4double minimum, G4double maximum)
    : G4VTrajectoryModel("particle_energy"),
      fMinimum(minimum),
      fMaximum(maximum),
      fLogMinimum(std::log(minimum)),
      fLogSpan(std::log(maximum) - fLogMinimum) {}

  void Draw(const G4VTrajectory& trajectory, const G4bool& visible) const override
  {
    auto* vis = G4VVisManager::GetConcreteInstance();
    if (!visible || !vis || trajectory.GetPointEntries() < 2) return;

    const auto* energyTrajectory = dynamic_cast<const EnergyTrajectory*>(&trajectory);
    const auto hue = BaseColour(trajectory.GetParticleName());
    G4Polyline line;
    G4double brightness = 0.0;
    const auto drawLine = [&] {
      if (line.size() < 2) return;
      G4VisAttributes attributes(ScaleColour(hue, brightness));
      attributes.SetLineWidth(2.0);
      line.SetVisAttributes(&attributes);
      vis->Draw(line);
      line.SetVisAttributes(nullptr);
    };

    auto start = trajectory.GetPoint(0)->GetPosition();
    for (G4int i = 1; i < trajectory.GetPointEntries(); ++i) {
      const auto end = trajectory.GetPoint(i)->GetPosition();
      if (start == end) continue;
      const auto energy = energyTrajectory ? energyTrajectory->SegmentEnergy(i) : missingEnergy;
      const auto nextBrightness = Brightness(energy);
      if (nextBrightness != brightness) {
        drawLine();
        line.clear();
        line.push_back(start);
        brightness = nextBrightness;
      }
      line.push_back(end);
      start = end;
    }
    drawLine();
  }

  void Print(std::ostream& out) const override
  {
    out << Name() << " (pre-step kinetic energy, " << fMinimum / MeV << " to "
        << fMaximum / MeV << " MeV)\n";
  }

  G4double Brightness(G4double energy) const
  {
    if (!std::isfinite(energy) || energy <= fMinimum) return 0.2;
    if (energy >= fMaximum) return 1.0;
    const auto fraction = fLogSpan > 0.0
      ? (std::log(energy) - fLogMinimum) / fLogSpan
      : (energy - fMinimum) / (fMaximum - fMinimum);
    return 0.2 + 0.8 * std::clamp(fraction, 0.0, 1.0);
  }

 private:
  const G4double fMinimum;
  const G4double fMaximum;
  const G4double fLogMinimum;
  const G4double fLogSpan;
};

}  // namespace

ParticleEnergyVisualization::ParticleEnergyVisualization(G4VisManager& visManager)
  : fVisManager(visManager),
    fMessenger(std::make_unique<G4GenericMessenger>(this, "/lunar/vis/"))
{
  fMessenger->DeclareMethod("particleEnergy", &ParticleEnergyVisualization::Configure,
                            "Select particle_energy with minimum and maximum in MeV before a run.")
    .SetParameterName(0, "minimumMeV", false, false)
    .SetParameterName(1, "maximumMeV", false, false)
    .SetRange("minimumMeV > 0 && maximumMeV > minimumMeV")
    .SetStates(G4State_PreInit)
    .SetToBeBroadcasted(false);
}

ParticleEnergyVisualization::~ParticleEnergyVisualization() = default;

void ParticleEnergyVisualization::Configure(G4double minimumMeV, G4double maximumMeV)
{
  if (RecordsStepEnergy() || !std::isfinite(minimumMeV) || !std::isfinite(maximumMeV)) return;
  fVisManager.RegisterModel(new ParticleEnergyModel(minimumMeV * MeV, maximumMeV * MeV));
  fRecordStepEnergy.store(true);
}

void ParticleEnergyTrackingAction::PreUserTrackingAction(const G4Track* track)
{
  if (fpTrackingManager->GetStoreTrajectory() && fVisualization.RecordsStepEnergy()) {
    fpTrackingManager->SetTrajectory(new EnergyTrajectory(*track));
  }
}

}  // namespace B1
