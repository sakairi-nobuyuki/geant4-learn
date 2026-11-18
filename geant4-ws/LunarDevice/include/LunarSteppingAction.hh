#ifndef LunarSteppingAction_h
#define LunarSteppingAction_h

#include "SteppingAction.hh"
#include "LunarRecorder.hh"

namespace B1 {
class LunarSteppingAction : public SteppingAction {
public:
  LunarSteppingAction(EventAction* eventAction, LunarRecorder& recorder)
    : SteppingAction(eventAction), fRecorder(recorder) {}
  void UserSteppingAction(const G4Step* step) override {
    // Preserve the B1 scoring mechanism and dose accumulation.
    SteppingAction::UserSteppingAction(step);
    fRecorder.ProcessStep(step);
  }
private:
  LunarRecorder& fRecorder;
};
} // namespace B1
#endif
