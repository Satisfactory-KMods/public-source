#include "Loader/KDFConditionEvaluator.h"

#include "KDFNode.h"
#include "Misc/EngineVersion.h"
#include "ModLoading/ModLoadingLibrary.h"
#include "Reflection/KDFValueCodec.h"
#include "Util/SemVersion.h"

namespace
{
	TArray<FString> NodeToStringList(const FKDFNode& Node)
	{
		TArray<FString> Result;
		if (Node.IsScalar())
		{
			Result.Add(Node.Scalar);
		}
		else if (Node.IsSequence())
		{
			for (const TSharedRef<FKDFNode>& Element : Node.Sequence)
			{
				Result.Add(Element->GetString());
			}
		}
		return Result;
	}

	bool EvaluateGameVersion(const FString& RangeString, UModLoadingLibrary* ModLoading, FString& OutFailReason)
	{
		FVersionRange Range;
		FString ParseError;
		if (!Range.ParseVersionRange(RangeString, ParseError))
		{
			OutFailReason = FString::Printf(TEXT("Invalid gameVersion range '%s': %s"), *RangeString, *ParseError);
			return false;
		}
		// SML reports the game as <changelist>.0.0 but GetLoadedModInfo("FactoryGame") returns false
		// (it falls through to a plugin lookup that cannot succeed for the project itself), so build
		// the same version directly.
		FModInfo GameInfo;
		if (!ModLoading->GetLoadedModInfo(TEXT("FactoryGame"), GameInfo))
		{
			GameInfo.Version = FVersion(FEngineVersion::Current().GetChangelist(), 0, 0);
		}
		if (!Range.Matches(GameInfo.Version))
		{
			OutFailReason = FString::Printf(TEXT("gameVersion %s does not match required %s"),
											*GameInfo.Version.ToString(), *RangeString);
			return false;
		}
		return true;
	}

	bool EvaluateHasMod(const FString& ModSpec, UModLoadingLibrary* ModLoading, FString& OutFailReason)
	{
		FString ModReference = ModSpec;
		FString RangeString;
		ModSpec.Split(TEXT("@"), &ModReference, &RangeString);
		ModReference.TrimStartAndEndInline();

		FModInfo ModInfo;
		if (!ModLoading->GetLoadedModInfo(ModReference, ModInfo))
		{
			OutFailReason = FString::Printf(TEXT("Required mod '%s' is not loaded"), *ModReference);
			return false;
		}
		if (!RangeString.IsEmpty())
		{
			FVersionRange Range;
			FString ParseError;
			if (!Range.ParseVersionRange(RangeString, ParseError))
			{
				OutFailReason =
					FString::Printf(TEXT("Invalid hasMod version range '%s': %s"), *RangeString, *ParseError);
				return false;
			}
			if (!Range.Matches(ModInfo.Version))
			{
				OutFailReason = FString::Printf(TEXT("Mod '%s' version %s does not match required %s"), *ModReference,
												*ModInfo.Version.ToString(), *RangeString);
				return false;
			}
		}
		return true;
	}
} // namespace

bool FKDFConditionEvaluator::Evaluate(const FKDFNode* ConditionsNode, UGameInstance* GameInstance,
									  FString& OutFailReason)
{
	if (ConditionsNode == nullptr || ConditionsNode->IsNull())
	{
		return true;
	}
	if (!ConditionsNode->IsMap())
	{
		OutFailReason = TEXT("'conditions' must be a map");
		return false;
	}
	UModLoadingLibrary* ModLoading = IsValid(GameInstance) ? GameInstance->GetSubsystem<UModLoadingLibrary>() : nullptr;

	for (const TTuple<FString, TSharedRef<FKDFNode>>& Condition : ConditionsNode->Map)
	{
		if (Condition.Key.Equals(TEXT("gameVersion"), ESearchCase::IgnoreCase))
		{
			if (ModLoading == nullptr)
			{
				OutFailReason = TEXT("Mod loading library unavailable for gameVersion check");
				return false;
			}
			if (!EvaluateGameVersion(Condition.Value->GetString(), ModLoading, OutFailReason))
			{
				return false;
			}
		}
		else if (Condition.Key.Equals(TEXT("hasMod"), ESearchCase::IgnoreCase))
		{
			if (ModLoading == nullptr)
			{
				OutFailReason = TEXT("Mod loading library unavailable for hasMod check");
				return false;
			}
			for (const FString& ModSpec : NodeToStringList(Condition.Value.Get()))
			{
				if (!EvaluateHasMod(ModSpec, ModLoading, OutFailReason))
				{
					return false;
				}
			}
		}
		else if (Condition.Key.Equals(TEXT("modVersion"), ESearchCase::IgnoreCase))
		{
			// Explicit map form: modVersion: { SML: ">=3.12.0", KAPI: "^2026.0.0" }
			if (ModLoading == nullptr)
			{
				OutFailReason = TEXT("Mod loading library unavailable for modVersion check");
				return false;
			}
			if (!Condition.Value->IsMap())
			{
				OutFailReason = TEXT("'modVersion' must be a map of mod reference to version range");
				return false;
			}
			for (const TTuple<FString, TSharedRef<FKDFNode>>& ModEntry : Condition.Value->Map)
			{
				const FString ModSpec = ModEntry.Key + TEXT("@") + ModEntry.Value->GetString();
				if (!EvaluateHasMod(ModSpec, ModLoading, OutFailReason))
				{
					return false;
				}
			}
		}
		else if (Condition.Key.Equals(TEXT("hasClass"), ESearchCase::IgnoreCase))
		{
			for (const FString& ClassPath : NodeToStringList(Condition.Value.Get()))
			{
				FString ClassError;
				if (FKDFValueCodec::ResolveClass(ClassPath, ClassError) == nullptr)
				{
					OutFailReason = FString::Printf(TEXT("Required class '%s' not found"), *ClassPath);
					return false;
				}
			}
		}
		else
		{
			OutFailReason = FString::Printf(TEXT("Unknown condition '%s'"), *Condition.Key);
			return false;
		}
	}
	return true;
}
