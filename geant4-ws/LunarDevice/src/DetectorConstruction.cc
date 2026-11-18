#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

namespace B1 {

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    auto* nist = G4NistManager::Instance();

    // Materials
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* aluminum = nist->FindOrBuildMaterial("G4_Al");
    auto* silicon = nist->FindOrBuildMaterial("G4_Si");

    // World: 600 x 600 x 600 mm
    auto* solidWorld = new G4Box(
        "World", 300*mm, 300*mm, 300*mm
    );
    auto* logicWorld = new G4LogicalVolume(
        solidWorld, vacuum, "World"
    );
    auto* physWorld = new G4PVPlacement(
        nullptr, {}, logicWorld, "World",
        nullptr, false, 0, true
    );

    // Aluminum enclosure: 100 x 100 x 30 mm
    auto* solidCase = new G4Box(
        "AlCase", 50*mm, 50*mm, 15*mm
    );
    auto* logicCase = new G4LogicalVolume(
        solidCase, aluminum, "AlCase"
    );
    new G4PVPlacement(
        nullptr, {}, logicCase, "AlCase",
        logicWorld, false, 0, true
    );

    // Inner cavity: 96 x 96 x 26 mm
    // This leaves 2 mm aluminum walls.
    auto* solidCavity = new G4Box(
        "Cavity", 48*mm, 48*mm, 13*mm
    );
    auto* logicCavity = new G4LogicalVolume(
        solidCavity, vacuum, "Cavity"
    );
    new G4PVPlacement(
        nullptr, {}, logicCavity, "Cavity",
        logicCase, false, 0, true
    );

    // Silicon chip: 10 x 10 x 0.5 mm
    auto* solidChip = new G4Box(
        "Chip", 5*mm, 5*mm, 0.25*mm
    );
    auto* logicChip = new G4LogicalVolume(
        solidChip, silicon, "Chip"
    );
    new G4PVPlacement(
        nullptr, {}, logicChip, "Chip",
        logicCavity, false, 0, true
    );

    // Energy deposition is measured inside silicon.
    fScoringVolume = logicChip;

    return physWorld;
}

} // namespace B1
