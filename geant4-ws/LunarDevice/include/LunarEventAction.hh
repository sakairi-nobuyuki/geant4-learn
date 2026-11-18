#ifndef LunarEventAction_h
#define LunarEventAction_h

#include "EventAction.hh"
#include "LunarRecorder.hh"

namespace B1 {
class LunarEventAction : public EventAction {
public:
  explicit LunarEventAction(RunAction* runAction) : EventAction(runAction) {}
  void BeginOfEventAction(const G4Event* event) override {
    EventAction::BeginOfEventAction(event);
    fRecorder.BeginEvent(event);
  }
  void EndOfEventAction(const G4Event* event) override {
    fRecorder.EndEvent(event);
    EventAction::EndOfEventAction(event);
  }
  LunarRecorder& Recorder() { return fRecorder; }
private:
  LunarRecorder fRecorder;
};
} // namespace B1
#endif
