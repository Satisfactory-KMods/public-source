#include "Subsystems/KBFLLocationSubsystem.h"

#include "BFL/KBFL_Util.h"

AKBFLLocationSubsystem::AKBFLLocationSubsystem() : LastClass(nullptr) { PrimaryActorTick.bCanEverTick = true; }

AKBFLLocationSubsystem* AKBFLLocationSubsystem::GetAKBFLLocationSubsystem(UObject* WorldContext)
{
	return Cast<AKBFLLocationSubsystem>(UKBFL_Util::GetSubsystem(WorldContext, StaticClass()));
}

void AKBFLLocationSubsystem::BeginPlay() { SaveLocationsToFile(); }

FString AKBFLLocationSubsystem::GetFilePath()
{
	FString file = FPaths::ProjectConfigDir();
	if (mAddFilePath != "")
	{
		file.Append(mAddFilePath);
	}

	return file;
}

void AKBFLLocationSubsystem::SaveLocationsToFile()
{
	for (TTuple<UClass*, FKBFLTransformArray> Result : Mapping)
	{
		UE_LOG(LogTemp, Warning, TEXT("Try SaveLocationsToFile"));

		FString FilePath = GetFilePath();

		FilePath.Append(Result.Key->GetName());
		FilePath.Append(".txt");

		const UScriptStruct* Struct = Result.Value.StaticStruct();
		FString FileContent;
		Struct->ExportText(FileContent, &Result.Value, new FKBFLTransformArray, this,
						   (PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited | PPF_IncludeTransient), nullptr);

		UE_LOG(LogTemp, Warning, TEXT("SaveLocationsToFile: %d"),
			   FFileHelper::SaveStringToFile(FileContent, *FilePath));
	}
}

void AKBFLLocationSubsystem::SaveLocation(UClass* ActorClass, FTransform Transform)
{
	if (Mapping.Contains(ActorClass))
	{
		Mapping[ActorClass].mTransforms.Add(Transform);
		SaveLocationsToFile();
		return;
	}

	FKBFLTransformArray Array;
	Array.mTransforms.Add(Transform);
	Mapping.Add(ActorClass, Array);
	SaveLocationsToFile();
}

void AKBFLLocationSubsystem::ClearLocation()
{
	for (TTuple<UClass*, FKBFLTransformArray> Result : Mapping)
	{
		Mapping.Add(Result.Key, FKBFLTransformArray());
	}
	SaveLocationsToFile();
}

void AKBFLLocationSubsystem::RemoveLocation(UClass* ActorClass, FTransform Transform)
{
	FKBFLTransformArray Array;
	if (Mapping.Contains(ActorClass))
	{
		for (FTransform MapTransform : Mapping[ActorClass].mTransforms)
		{
			if (FVector::Distance(MapTransform.GetLocation(), Transform.GetLocation()) > 2.f)
			{
				Array.mTransforms.Add(MapTransform);
			}
		}
	}

	Mapping.Add(ActorClass, Array);
	SaveLocationsToFile();
}
