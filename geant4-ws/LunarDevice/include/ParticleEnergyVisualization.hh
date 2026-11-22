#ifndef ParticleEnergyVisualization_hh
#define ParticleEnergyVisualization_hh

#include "G4UserTrackingAction.hh"
#include "G4Types.hh"

#include <atomic>
#include <memory>

class G4GenericMessenger;
class G4VisManager;

namespace B1 {

class ParticleEnergyVisualization {
 public:
  explicit ParticleEnergyVisualization(G4VisManager& visManager);
  ~ParticleEnergyVisualization();

  bool RecordsStepEnergy() const { return fRecordStepEnergy.load(); }

 private:
  void Configure(G4double minimumMeV, G4double maximumMeV);

  G4VisManager& fVisManager;
  std::atomic<bool> fRecordStepEnergy{false};
  std::unique_ptr<G4GenericMessenger> fMessenger;
};

class ParticleEnergyTrackingAction final : public G4UserTrackingAction {
 public:
  explicit ParticleEnergyTrackingAction(const ParticleEnergyVisualization& visualization)
    : fVisualization(visualization) {}

  void PreUserTrackingAction(const G4Track* track) override;

 private:
  const ParticleEnergyVisualization& fVisualization;
};

}  // namespace B1

#endif
