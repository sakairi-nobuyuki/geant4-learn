#include "ActionInitialization.hh"
#include "LunarEventAction.hh"
#include "LunarSteppingAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"

namespace B1 {
void ActionInitialization::BuildForMaster() const {
  SetUserAction(new RunAction);
}
void ActionInitialization::Build() const {
  SetUserAction(new PrimaryGeneratorAction);
  auto* runAction = new RunAction;
  SetUserAction(runAction);
  auto* eventAction = new LunarEventAction(runAction);
  SetUserAction(eventAction);
  runAction->SetRecorder(eventAction->Recorder());
  SetUserAction(new LunarSteppingAction(eventAction, eventAction->Recorder()));
}
} // namespace B1
