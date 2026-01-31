#include "Subsystems/HelperClasses/KBFLWorldCDOCallRequirement.h"

// Implementation for world-specific requirements
// Add custom implementation here when needed
void UKBFLWorldCDOCallRequirement::Tick_Implementation(float dt, class UKBFLCDOOverwriteWorldBasedBase* From) {}
class UWorld* UKBFLWorldCDOCallRequirement::GetWorld() const { return mWorld; }
