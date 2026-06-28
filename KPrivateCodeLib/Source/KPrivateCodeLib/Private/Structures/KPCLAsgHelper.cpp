// ILikeBanas

#include "Structures/KPCLAsgHelper.h"

#include "KPrivateCodeLibModule.h"

void UKPCLAsgHelperListener::HandleOptionUpdated(FString UpdatedCVar)
{
	if (mHelper)
	{
		UE_LOG(LogKPCL, Verbose, TEXT("UKPCLAsgHelperListener::HandleOptionUpdated — cvar='%s' key='%s' world=%s"),
			   *UpdatedCVar, *mHelper->GetKey(), mWorld.IsValid() ? *mWorld->GetName() : TEXT("<null>"));
		mHelper->OnUpdate(mWorld.Get(), UpdatedCVar);
	}
	else
	{
		UE_LOG(LogKPCL, Warning,
			   TEXT("UKPCLAsgHelperListener::HandleOptionUpdated — mHelper is null (cvar='%s'), listener was not "
					"cleaned up properly"),
			   *UpdatedCVar);
	}
}
