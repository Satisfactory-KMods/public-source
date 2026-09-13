// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Loader/KDFConditionEvaluator.h"

#include "KDFNode.h"
#include "Misc/EngineVersion.h"
#include "ModLoading/ModLoadingLibrary.h"
#include "Reflection/KDFValueCodec.h"
#include "Util/SemVersion.h"

namespace
{
	enum class EKDFConditionResult : uint8
	{
		Match,
		NoMatch,
		Error
	};

	enum class EKDFConditionBehavior : uint8
	{
		And,
		Or
	};

	bool ParseConditionBehavior(const FKDFNode* BehaviorNode, EKDFConditionBehavior& OutBehavior,
								FString& OutFailReason)
	{
		OutBehavior = EKDFConditionBehavior::And;
		if (BehaviorNode == nullptr || BehaviorNode->IsNull())
		{
			return true;
		}
		if (!BehaviorNode->IsScalar())
		{
			OutFailReason = TEXT("'conditionBehaivor' must be AND or OR");
			return false;
		}
		if (BehaviorNode->Scalar.Equals(TEXT("AND"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		if (BehaviorNode->Scalar.Equals(TEXT("OR"), ESearchCase::IgnoreCase))
		{
			OutBehavior = EKDFConditionBehavior::Or;
			return true;
		}
		OutFailReason =
			FString::Printf(TEXT("Invalid conditionBehaivor '%s'; expected AND or OR"), *BehaviorNode->Scalar);
		return false;
	}

	bool NodeToStringList(const FKDFNode& Node, const TCHAR* ConditionName, TArray<FString>& OutResult,
						  FString& OutFailReason)
	{
		if (Node.IsScalar())
		{
			OutResult.Add(Node.Scalar);
			return true;
		}
		if (!Node.IsSequence())
		{
			OutFailReason = FString::Printf(TEXT("'%s' must be a scalar or sequence"), ConditionName);
			return false;
		}
		for (const TSharedRef<FKDFNode>& Element : Node.Sequence)
		{
			if (!Element->IsScalar())
			{
				OutFailReason = FString::Printf(TEXT("'%s' sequence entries must be scalars"), ConditionName);
				return false;
			}
			OutResult.Add(Element->Scalar);
		}
		return true;
	}

	bool ParseModSpec(const FString& ModSpec, const TCHAR* ConditionName, FString& OutModReference,
					  FString& OutRangeString, FVersionRange& OutRange, bool& bOutHasRange, FString& OutFailReason)
	{
		OutModReference = ModSpec;
		OutRangeString.Reset();
		bOutHasRange = ModSpec.Split(TEXT("@"), &OutModReference, &OutRangeString);
		OutModReference.TrimStartAndEndInline();
		OutRangeString.TrimStartAndEndInline();

		if (OutModReference.IsEmpty())
		{
			OutFailReason = FString::Printf(TEXT("'%s' mod reference must not be empty"), ConditionName);
			return false;
		}
		if (!bOutHasRange)
		{
			return true;
		}
		if (OutRangeString.IsEmpty())
		{
			OutFailReason = FString::Printf(TEXT("'%s' version range must not be empty"), ConditionName);
			return false;
		}

		FString ParseError;
		if (!OutRange.ParseVersionRange(OutRangeString, ParseError))
		{
			OutFailReason =
				FString::Printf(TEXT("Invalid %s version range '%s': %s"), ConditionName, *OutRangeString, *ParseError);
			return false;
		}
		return true;
	}

	EKDFConditionResult EvaluateGameVersion(const FString& RangeString, UModLoadingLibrary* ModLoading,
											FString& OutFailReason)
	{
		FVersionRange Range;
		FString ParseError;
		if (!Range.ParseVersionRange(RangeString, ParseError))
		{
			OutFailReason = FString::Printf(TEXT("Invalid gameVersion range '%s': %s"), *RangeString, *ParseError);
			return EKDFConditionResult::Error;
		}

		FModInfo GameInfo;
		if (!ModLoading->GetLoadedModInfo(TEXT("FactoryGame"), GameInfo))
		{
			GameInfo.Version = FVersion(FEngineVersion::Current().GetChangelist(), 0, 0);
		}
		if (!Range.Matches(GameInfo.Version))
		{
			OutFailReason = FString::Printf(TEXT("gameVersion %s does not match required %s"),
											*GameInfo.Version.ToString(), *RangeString);
			return EKDFConditionResult::NoMatch;
		}
		return EKDFConditionResult::Match;
	}

	EKDFConditionResult EvaluateHasMod(const FString& ModSpec, const TCHAR* ConditionName,
									   UModLoadingLibrary* ModLoading, FString& OutFailReason)
	{
		FString ModReference;
		FString RangeString;
		FVersionRange Range;
		bool bHasRange = false;
		if (!ParseModSpec(ModSpec, ConditionName, ModReference, RangeString, Range, bHasRange, OutFailReason))
		{
			return EKDFConditionResult::Error;
		}

		FModInfo ModInfo;
		if (!ModLoading->GetLoadedModInfo(ModReference, ModInfo))
		{
			OutFailReason = FString::Printf(TEXT("Required mod '%s' is not loaded"), *ModReference);
			return EKDFConditionResult::NoMatch;
		}
		if (bHasRange && !Range.Matches(ModInfo.Version))
		{
			OutFailReason = FString::Printf(TEXT("Mod '%s' version %s does not match required %s"), *ModReference,
											*ModInfo.Version.ToString(), *RangeString);
			return EKDFConditionResult::NoMatch;
		}
		return EKDFConditionResult::Match;
	}

	EKDFConditionResult EvaluateAllModSpecs(const TArray<FString>& ModSpecs, const TCHAR* ConditionName,
											UModLoadingLibrary* ModLoading, FString& OutFailReason)
	{
		OutFailReason.Reset();
		EKDFConditionResult Result = EKDFConditionResult::Match;
		for (const FString& ModSpec : ModSpecs)
		{
			FString ModReason;
			const EKDFConditionResult ModResult = EvaluateHasMod(ModSpec, ConditionName, ModLoading, ModReason);
			if (ModResult == EKDFConditionResult::Error)
			{
				OutFailReason = MoveTemp(ModReason);
				return EKDFConditionResult::Error;
			}
			if (ModResult == EKDFConditionResult::NoMatch)
			{
				Result = EKDFConditionResult::NoMatch;
				if (OutFailReason.IsEmpty())
				{
					OutFailReason = MoveTemp(ModReason);
				}
			}
		}
		return Result;
	}

	EKDFConditionResult EvaluateConditionObject(const FKDFNode& ConditionObject, UGameInstance* GameInstance,
												FString& OutFailReason)
	{
		OutFailReason.Reset();
		if (!ConditionObject.IsMap())
		{
			OutFailReason = TEXT("Condition entries must be maps");
			return EKDFConditionResult::Error;
		}

		bool bIfNotMatch = false;
		bool bFoundIfNotMatch = false;
		for (const TTuple<FString, TSharedRef<FKDFNode>>& Condition : ConditionObject.Map)
		{
			if (!Condition.Key.Equals(TEXT("ifNotMatch"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			if (bFoundIfNotMatch)
			{
				OutFailReason = TEXT("'ifNotMatch' may only appear once per condition object");
				return EKDFConditionResult::Error;
			}
			bFoundIfNotMatch = true;
			if (!Condition.Value->TryGetBool(bIfNotMatch))
			{
				OutFailReason = TEXT("'ifNotMatch' must be a boolean");
				return EKDFConditionResult::Error;
			}
		}

		UModLoadingLibrary* ModLoading =
			IsValid(GameInstance) ? GameInstance->GetSubsystem<UModLoadingLibrary>() : nullptr;
		bool bAllMatched = true;
		FString FirstMismatchReason;

		for (const TTuple<FString, TSharedRef<FKDFNode>>& Condition : ConditionObject.Map)
		{
			if (Condition.Key.Equals(TEXT("ifNotMatch"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			FString ConditionReason;
			EKDFConditionResult Result = EKDFConditionResult::Error;
			if (Condition.Key.Equals(TEXT("gameVersion"), ESearchCase::IgnoreCase))
			{
				if (ModLoading == nullptr)
				{
					ConditionReason = TEXT("Mod loading library unavailable for gameVersion check");
				}
				else
				{
					Result = EvaluateGameVersion(Condition.Value->GetString(), ModLoading, ConditionReason);
				}
			}
			else if (Condition.Key.Equals(TEXT("hasMod"), ESearchCase::IgnoreCase))
			{
				if (ModLoading == nullptr)
				{
					ConditionReason = TEXT("Mod loading library unavailable for hasMod check");
				}
				else
				{
					Result = EKDFConditionResult::Match;
					TArray<FString> ModSpecs;
					if (!NodeToStringList(Condition.Value.Get(), TEXT("hasMod"), ModSpecs, ConditionReason))
					{
						Result = EKDFConditionResult::Error;
					}
					else
					{
						Result = EvaluateAllModSpecs(ModSpecs, TEXT("hasMod"), ModLoading, ConditionReason);
					}
				}
			}
			else if (Condition.Key.Equals(TEXT("modVersion"), ESearchCase::IgnoreCase))
			{
				if (ModLoading == nullptr)
				{
					ConditionReason = TEXT("Mod loading library unavailable for modVersion check");
				}
				else if (!Condition.Value->IsMap())
				{
					ConditionReason = TEXT("'modVersion' must be a map of mod reference to version range");
				}
				else
				{
					Result = EKDFConditionResult::Match;
					TArray<FString> ModSpecs;
					for (const TTuple<FString, TSharedRef<FKDFNode>>& ModEntry : Condition.Value->Map)
					{
						if (!ModEntry.Value->IsScalar())
						{
							ConditionReason = TEXT("'modVersion' ranges must be scalars");
							Result = EKDFConditionResult::Error;
							break;
						}
						ModSpecs.Add(ModEntry.Key + TEXT("@") + ModEntry.Value->Scalar);
					}
					if (Result != EKDFConditionResult::Error)
					{
						Result = EvaluateAllModSpecs(ModSpecs, TEXT("modVersion"), ModLoading, ConditionReason);
					}
				}
			}
			else if (Condition.Key.Equals(TEXT("hasClass"), ESearchCase::IgnoreCase))
			{
				TArray<FString> ClassPaths;
				if (!NodeToStringList(Condition.Value.Get(), TEXT("hasClass"), ClassPaths, ConditionReason))
				{
					Result = EKDFConditionResult::Error;
				}
				else
				{
					Result = EKDFConditionResult::Match;
					for (const FString& ClassPath : ClassPaths)
					{
						FString ClassError;
						if (FKDFValueCodec::ResolveClass(ClassPath, ClassError) == nullptr)
						{
							ConditionReason = FString::Printf(TEXT("Required class '%s' not found"), *ClassPath);
							Result = EKDFConditionResult::NoMatch;
							break;
						}
					}
				}
			}
			else
			{
				ConditionReason = FString::Printf(TEXT("Unknown condition '%s'"), *Condition.Key);
			}

			if (Result == EKDFConditionResult::Error)
			{
				OutFailReason = MoveTemp(ConditionReason);
				return EKDFConditionResult::Error;
			}
			if (Result == EKDFConditionResult::NoMatch)
			{
				bAllMatched = false;
				if (FirstMismatchReason.IsEmpty())
				{
					FirstMismatchReason = MoveTemp(ConditionReason);
				}
			}
		}

		if (bIfNotMatch)
		{
			if (!bAllMatched)
			{
				return EKDFConditionResult::Match;
			}
			OutFailReason = TEXT("Condition object matched but ifNotMatch requires it not to match");
			return EKDFConditionResult::NoMatch;
		}
		if (!bAllMatched)
		{
			OutFailReason = MoveTemp(FirstMismatchReason);
			return EKDFConditionResult::NoMatch;
		}
		return EKDFConditionResult::Match;
	}
}

bool FKDFConditionEvaluator::Evaluate(const FKDFNode* ConditionsNode, const FKDFNode* ConditionBehaviorNode,
									  UGameInstance* GameInstance, FString& OutFailReason)
{
	OutFailReason.Reset();
	EKDFConditionBehavior Behavior = EKDFConditionBehavior::And;
	if (!ParseConditionBehavior(ConditionBehaviorNode, Behavior, OutFailReason))
	{
		return false;
	}
	if (ConditionsNode == nullptr || ConditionsNode->IsNull())
	{
		return true;
	}
	if (ConditionsNode->IsMap())
	{
		return EvaluateConditionObject(*ConditionsNode, GameInstance, OutFailReason) == EKDFConditionResult::Match;
	}
	if (!ConditionsNode->IsSequence())
	{
		OutFailReason = TEXT("'conditions' must be a map or sequence of maps");
		return false;
	}

	bool bAllMatched = true;
	bool bAnyMatched = false;
	FString FirstMismatchReason;
	for (int32 Index = 0; Index < ConditionsNode->Sequence.Num(); ++Index)
	{
		const FKDFNode& ConditionObject = ConditionsNode->Sequence[Index].Get();
		if (!ConditionObject.IsMap())
		{
			OutFailReason = FString::Printf(TEXT("conditions[%d] must be a map"), Index);
			return false;
		}
		FString GroupFailReason;
		const EKDFConditionResult GroupResult = EvaluateConditionObject(ConditionObject, GameInstance, GroupFailReason);
		if (GroupResult == EKDFConditionResult::Error)
		{
			OutFailReason = FString::Printf(TEXT("conditions[%d]: %s"), Index, *GroupFailReason);
			return false;
		}
		if (GroupResult == EKDFConditionResult::Match)
		{
			bAnyMatched = true;
		}
		else
		{
			bAllMatched = false;
			if (FirstMismatchReason.IsEmpty())
			{
				FirstMismatchReason = FString::Printf(TEXT("conditions[%d]: %s"), Index, *GroupFailReason);
			}
		}
	}

	if (Behavior == EKDFConditionBehavior::Or)
	{
		if (bAnyMatched)
		{
			return true;
		}
		OutFailReason = FirstMismatchReason.IsEmpty() ? TEXT("No condition object matched; conditions is empty")
													  : MoveTemp(FirstMismatchReason);
		return false;
	}
	if (!bAllMatched)
	{
		OutFailReason = MoveTemp(FirstMismatchReason);
		return false;
	}
	return true;
}
