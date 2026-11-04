#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

namespace B1 {

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
    fParticleGun = new G4ParticleGun(1);

    auto* particle =
        G4ParticleTable::GetParticleTable()
            ->FindParticle("proton");

    fParticleGun->SetParticleDefinition(particle);
    fParticleGun->SetParticleEnergy(100*MeV);
    fParticleGun->SetParticleMomentumDirection(
        G4ThreeVector(0, 0, -1)
    );
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    // Uniform source across the chip's projected area.
    G4double x = (G4UniformRand() - 0.5)*10*mm;
    G4double y = (G4UniformRand() - 0.5)*10*mm;

    // Beam starts 100 mm above enclosure center.
    fParticleGun->SetParticlePosition(
        G4ThreeVector(x, y, 100*mm)
    );

    fParticleGun->GeneratePrimaryVertex(event);
}

} // namespace B1