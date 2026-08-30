#include "Subsystems/HelperClasses/KBFLWorldCDOCallRequirement.h"

void UKBFLWorldCDOCallRequirement::Tick_Implementation(float dt, class UKBFLCDOOverwriteWorldBasedBase* From) {}
class UWorld* UKBFLWorldCDOCallRequirement::GetWorld() const { return mWorld; }
