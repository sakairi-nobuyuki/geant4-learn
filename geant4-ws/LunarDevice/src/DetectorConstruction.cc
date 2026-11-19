#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace B1 {

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    constexpr auto caseHalfXY = 50*mm;
    constexpr auto caseHalfZ = 15*mm;
    auto thickness = 2*mm;
    if (const char* value = std::getenv("LUNAR_AL_THICKNESS_MM")) {
        char* end = nullptr;
        errno = 0;
        const auto thicknessMm = std::strtod(value, &end);
        if (end == value || *end != '\0' || errno == ERANGE || !std::isfinite(thicknessMm) ||
            thicknessMm < 0 || thicknessMm >= caseHalfZ/mm ||
            (thicknessMm > 0 && caseHalfZ - thicknessMm*mm <= 0.25*mm)) {
            G4ExceptionDescription description;
            description << "Invalid LUNAR_AL_THICKNESS_MM='" << value
                        << "'. Use 0 for an unshielded reference, or a positive"
                        << " thickness below 14.75 mm to leave room for the chip.";
            G4Exception("DetectorConstruction::Construct", "LunarGeometry001",
                        FatalException, description);
        }
        thickness = thicknessMm*mm;
    }

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
    auto* cavityMother = logicWorld;
    if (thickness > 0) {
        auto* solidCase = new G4Box("AlCase", caseHalfXY, caseHalfXY, caseHalfZ);
        auto* logicCase = new G4LogicalVolume(solidCase, aluminum, "AlCase");
        new G4PVPlacement(nullptr, {}, logicCase, "AlCase",
                          logicWorld, false, 0, true);
        cavityMother = logicCase;
    }

    // With no wall, the vacuum cavity fills the enclosure footprint.
    auto* solidCavity = new G4Box(
        "Cavity", caseHalfXY - thickness, caseHalfXY - thickness, caseHalfZ - thickness
    );
    auto* logicCavity = new G4LogicalVolume(
        solidCavity, vacuum, "Cavity"
    );
    new G4PVPlacement(
        nullptr, {}, logicCavity, "Cavity",
        cavityMother, false, 0, true
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
