// 

#include "Network/Buildings/KPCLNetworkTower.h"

#include "KPrivateCodeLibModule.h"

FText AKPCLNetworkTower::GetActorRepresentationText()
{
	FText SuperName = Super::GetActorRepresentationText();
	return FText::FromString(FString(SuperName.ToString()).Append(" (Network Tower)"));
}

void AKPCLNetworkTower::BeginPlay()
{
	Super::BeginPlay();

	AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
	if (IsValid(FaxitSubsystem))
	{
		FaxitSubsystem->RegisterNetworkTower(this);
	}

	TryToConnectToNearstCore();
}

void AKPCLNetworkTower::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AKPCLFaxitSubsystem* FaxitSubsystem = AKPCLFaxitSubsystem::Get(GetWorld());
	if (IsValid(FaxitSubsystem))
	{
		FaxitSubsystem->UnRegisterNetworkTower(this);
	}
}

void AKPCLNetworkTower::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AKPCLNetworkTower::Factory_Tick(float dt)
{
	Super::Factory_Tick(dt);
	if (HasAuthority() && !mConnectedRadarTower.IsValid())
	{
		AsyncTask(ENamedThreads::Type::GameThread, [this]
		{
			Execute_Dismantle(this);
		});
	}
}

// Sets default values
AKPCLNetworkTower::AKPCLNetworkTower()
{
}
