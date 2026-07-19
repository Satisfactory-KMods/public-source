#include "Handlers/KDFCurveHandler.h"

#include "Content/KDFDynamicContent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Engine/GameInstance.h"
#include "KDFLogging.h"
#include "KDFYamlParser.h"
#include "Reflection/KDFPatchUtil.h"
#include "Subsystems/KDFSubsystem.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	const FKDFNode* FindCaseInsensitive(const FKDFNode& Map, const TCHAR* Key)
	{
		if (!Map.IsMap())
		{
			return nullptr;
		}
		for (const TTuple<FString, TSharedRef<FKDFNode>>& Pair : Map.Map)
		{
			if (Pair.Key.Equals(Key, ESearchCase::IgnoreCase))
			{
				return &Pair.Value.Get();
			}
		}
		return nullptr;
	}

	bool TryReadNumber(const FKDFNode* Node, float& OutValue)
	{
		double Value = 0.0;
		if (Node == nullptr || !Node->TryGetFloat(Value) || !FMath::IsFinite(Value))
		{
			return false;
		}
		OutValue = static_cast<float>(Value);
		return FMath::IsFinite(OutValue);
	}

	bool TryReadVector(const FKDFNode* Node, FVector3f& OutValue)
	{
		if (Node == nullptr)
		{
			return false;
		}
		if (Node->IsSequence() && Node->Num() == 3)
		{
			return TryReadNumber(&Node->Sequence[0].Get(), OutValue.X) &&
				TryReadNumber(&Node->Sequence[1].Get(), OutValue.Y) &&
				TryReadNumber(&Node->Sequence[2].Get(), OutValue.Z);
		}
		if (!Node->IsMap())
		{
			return false;
		}
		return TryReadNumber(FindCaseInsensitive(*Node, TEXT("X")), OutValue.X) &&
			TryReadNumber(FindCaseInsensitive(*Node, TEXT("Y")), OutValue.Y) &&
			TryReadNumber(FindCaseInsensitive(*Node, TEXT("Z")), OutValue.Z);
	}

	bool ParseInterpMode(const FString& Value, ERichCurveInterpMode& OutMode)
	{
		if (Value.Equals(TEXT("linear"), ESearchCase::IgnoreCase))
		{
			OutMode = RCIM_Linear;
			return true;
		}
		if (Value.Equals(TEXT("constant"), ESearchCase::IgnoreCase))
		{
			OutMode = RCIM_Constant;
			return true;
		}
		if (Value.Equals(TEXT("cubic"), ESearchCase::IgnoreCase))
		{
			OutMode = RCIM_Cubic;
			return true;
		}
		return false;
	}

	bool ParseTangentMode(const FString& Value, ERichCurveTangentMode& OutMode)
	{
		if (Value.Equals(TEXT("auto"), ESearchCase::IgnoreCase))
		{
			OutMode = RCTM_Auto;
			return true;
		}
		if (Value.Equals(TEXT("user"), ESearchCase::IgnoreCase))
		{
			OutMode = RCTM_User;
			return true;
		}
		if (Value.Equals(TEXT("break"), ESearchCase::IgnoreCase))
		{
			OutMode = RCTM_Break;
			return true;
		}
		if (Value.Equals(TEXT("smartAuto"), ESearchCase::IgnoreCase))
		{
			OutMode = RCTM_SmartAuto;
			return true;
		}
		return false;
	}

	bool ParseTangentWeightMode(const FString& Value, ERichCurveTangentWeightMode& OutMode)
	{
		if (Value.Equals(TEXT("none"), ESearchCase::IgnoreCase))
		{
			OutMode = RCTWM_WeightedNone;
			return true;
		}
		if (Value.Equals(TEXT("arrive"), ESearchCase::IgnoreCase))
		{
			OutMode = RCTWM_WeightedArrive;
			return true;
		}
		if (Value.Equals(TEXT("leave"), ESearchCase::IgnoreCase))
		{
			OutMode = RCTWM_WeightedLeave;
			return true;
		}
		if (Value.Equals(TEXT("both"), ESearchCase::IgnoreCase))
		{
			OutMode = RCTWM_WeightedBoth;
			return true;
		}
		return false;
	}

	bool ParseExtrapolation(const FString& Value, ERichCurveExtrapolation& OutMode)
	{
		if (Value.Equals(TEXT("none"), ESearchCase::IgnoreCase))
		{
			OutMode = RCCE_None;
			return true;
		}
		if (Value.Equals(TEXT("constant"), ESearchCase::IgnoreCase))
		{
			OutMode = RCCE_Constant;
			return true;
		}
		if (Value.Equals(TEXT("linear"), ESearchCase::IgnoreCase))
		{
			OutMode = RCCE_Linear;
			return true;
		}
		if (Value.Equals(TEXT("cycle"), ESearchCase::IgnoreCase))
		{
			OutMode = RCCE_Cycle;
			return true;
		}
		if (Value.Equals(TEXT("cycleWithOffset"), ESearchCase::IgnoreCase))
		{
			OutMode = RCCE_CycleWithOffset;
			return true;
		}
		if (Value.Equals(TEXT("oscillate"), ESearchCase::IgnoreCase))
		{
			OutMode = RCCE_Oscillate;
			return true;
		}
		return false;
	}

	bool ReadScalarOrVector(const FKDFNode& Key, const TCHAR* Field, const bool bVector, FVector3f& OutValue,
							const FVector3f& DefaultValue, FString& OutError)
	{
		const FKDFNode* Node = Key.Find(Field);
		if (Node == nullptr)
		{
			OutValue = DefaultValue;
			return true;
		}
		if (bVector)
		{
			if (TryReadVector(Node, OutValue))
			{
				return true;
			}
			OutError = FString::Printf(TEXT("%s must be an {X,Y,Z} map or three-number sequence"), Field);
			return false;
		}

		float Scalar = 0.0f;
		if (!TryReadNumber(Node, Scalar))
		{
			OutError = FString::Printf(TEXT("%s must be a finite number"), Field);
			return false;
		}
		OutValue = FVector3f(Scalar);
		return true;
	}

	bool BuildCurveKeys(const FKDFNode& Entry, const bool bVector, TArray<FRichCurveKey> OutKeys[3], FString& OutError)
	{
		const FKDFNode* Keys = Entry.Find(TEXT("keys"));
		if (Keys == nullptr || !Keys->IsSequence() || Keys->Num() == 0)
		{
			OutError = TEXT("keys must be a non-empty sequence");
			return false;
		}

		for (int32 Index = 0; Index < Keys->Sequence.Num(); ++Index)
		{
			const FKDFNode& Key = Keys->Sequence[Index].Get();
			if (!Key.IsMap())
			{
				OutError = FString::Printf(TEXT("keys[%d] must be a map"), Index);
				return false;
			}

			float Time = 0.0f;
			if (!TryReadNumber(Key.Find(TEXT("time")), Time))
			{
				OutError = FString::Printf(TEXT("keys[%d].time must be a finite number"), Index);
				return false;
			}
			if (Key.Find(TEXT("value")) == nullptr)
			{
				OutError = FString::Printf(TEXT("keys[%d].value is required"), Index);
				return false;
			}
			FVector3f Value;
			if (!ReadScalarOrVector(Key, TEXT("value"), bVector, Value, FVector3f::ZeroVector, OutError))
			{
				OutError = FString::Printf(TEXT("keys[%d].%s"), Index, *OutError);
				return false;
			}

			ERichCurveInterpMode InterpMode = RCIM_Linear;
			if (const FKDFNode* Mode = Key.Find(TEXT("interpMode"));
				Mode != nullptr && (!Mode->IsScalar() || !ParseInterpMode(Mode->Scalar, InterpMode)))
			{
				OutError = FString::Printf(TEXT("keys[%d].interpMode must be constant, linear, or cubic"), Index);
				return false;
			}
			ERichCurveTangentMode TangentMode = RCTM_Auto;
			if (const FKDFNode* Mode = Key.Find(TEXT("tangentMode"));
				Mode != nullptr && (!Mode->IsScalar() || !ParseTangentMode(Mode->Scalar, TangentMode)))
			{
				OutError = FString::Printf(TEXT("keys[%d].tangentMode must be auto, smartAuto, user, or break"), Index);
				return false;
			}
			ERichCurveTangentWeightMode WeightMode = RCTWM_WeightedNone;
			if (const FKDFNode* Mode = Key.Find(TEXT("tangentWeightMode"));
				Mode != nullptr && (!Mode->IsScalar() || !ParseTangentWeightMode(Mode->Scalar, WeightMode)))
			{
				OutError =
					FString::Printf(TEXT("keys[%d].tangentWeightMode must be none, arrive, leave, or both"), Index);
				return false;
			}

			FVector3f ArriveTangent;
			FVector3f LeaveTangent;
			FVector3f ArriveWeight;
			FVector3f LeaveWeight;
			if (!ReadScalarOrVector(Key, TEXT("arriveTangent"), bVector, ArriveTangent, FVector3f::ZeroVector,
									OutError) ||
				!ReadScalarOrVector(Key, TEXT("leaveTangent"), bVector, LeaveTangent, FVector3f::ZeroVector,
									OutError) ||
				!ReadScalarOrVector(Key, TEXT("arriveTangentWeight"), bVector, ArriveWeight, FVector3f::ZeroVector,
									OutError) ||
				!ReadScalarOrVector(Key, TEXT("leaveTangentWeight"), bVector, LeaveWeight, FVector3f::ZeroVector,
									OutError))
			{
				OutError = FString::Printf(TEXT("keys[%d].%s"), Index, *OutError);
				return false;
			}

			for (int32 Channel = 0; Channel < (bVector ? 3 : 1); ++Channel)
			{
				FRichCurveKey RichKey(Time, Value[Channel]);
				RichKey.InterpMode = InterpMode;
				RichKey.TangentMode = TangentMode;
				RichKey.TangentWeightMode = WeightMode;
				RichKey.ArriveTangent = ArriveTangent[Channel];
				RichKey.LeaveTangent = LeaveTangent[Channel];
				RichKey.ArriveTangentWeight = ArriveWeight[Channel];
				RichKey.LeaveTangentWeight = LeaveWeight[Channel];
				OutKeys[Channel].Add(RichKey);
			}
		}

		for (int32 Channel = 0; Channel < (bVector ? 3 : 1); ++Channel)
		{
			OutKeys[Channel].Sort([](const FRichCurveKey& A, const FRichCurveKey& B) { return A.Time < B.Time; });
			for (int32 Index = 1; Index < OutKeys[Channel].Num(); ++Index)
			{
				if (OutKeys[Channel][Index - 1].Time == OutKeys[Channel][Index].Time)
				{
					OutError = FString::Printf(TEXT("duplicate key time %s"),
											   *FString::SanitizeFloat(OutKeys[Channel][Index].Time));
					return false;
				}
			}
		}
		return true;
	}

	bool ValidateCurveMetadata(const FKDFNode& Entry, const bool bVector, FString& OutError)
	{
		for (const TCHAR* Field : {TEXT("preInfinityExtrap"), TEXT("postInfinityExtrap")})
		{
			if (const FKDFNode* Node = Entry.Find(Field))
			{
				ERichCurveExtrapolation Mode;
				if (!Node->IsScalar() || !ParseExtrapolation(Node->Scalar, Mode))
				{
					OutError = FString::Printf(
						TEXT("%s must be none, constant, linear, cycle, cycleWithOffset, or oscillate"), Field);
					return false;
				}
			}
		}
		if (const FKDFNode* DefaultValue = Entry.Find(TEXT("defaultValue")))
		{
			FVector3f Parsed;
			if ((bVector && !TryReadVector(DefaultValue, Parsed)) ||
				(!bVector && !TryReadNumber(DefaultValue, Parsed.X)))
			{
				OutError = bVector ? TEXT("defaultValue must be an {X,Y,Z} map or three-number sequence")
								   : TEXT("defaultValue must be a finite number");
				return false;
			}
		}
		if (bVector && Entry.Find(TEXT("isEventCurve")) != nullptr)
		{
			OutError = TEXT("isEventCurve is only valid for float curves");
			return false;
		}
		if (const FKDFNode* IsEventCurve = Entry.Find(TEXT("isEventCurve")))
		{
			bool bValue = false;
			if (!IsEventCurve->TryGetBool(bValue))
			{
				OutError = TEXT("isEventCurve must be boolean");
				return false;
			}
		}
		return true;
	}

	void ApplyMetadata(FRichCurve& Curve, const FKDFNode& Entry, const int32 Channel, const bool bVector)
	{
		if (const FKDFNode* Node = Entry.Find(TEXT("preInfinityExtrap")))
		{
			ERichCurveExtrapolation Mode;
			ParseExtrapolation(Node->Scalar, Mode);
			Curve.PreInfinityExtrap = Mode;
		}
		if (const FKDFNode* Node = Entry.Find(TEXT("postInfinityExtrap")))
		{
			ERichCurveExtrapolation Mode;
			ParseExtrapolation(Node->Scalar, Mode);
			Curve.PostInfinityExtrap = Mode;
		}
		if (const FKDFNode* Node = Entry.Find(TEXT("defaultValue")))
		{
			if (bVector)
			{
				FVector3f Value;
				TryReadVector(Node, Value);
				Curve.SetDefaultValue(Value[Channel]);
			}
			else
			{
				float Value = 0.0f;
				TryReadNumber(Node, Value);
				Curve.SetDefaultValue(Value);
			}
		}
	}

	void ApplyFloatCurve(UCurveFloat* Curve, const FKDFNode& Entry, const TArray<FRichCurveKey>& Keys)
	{
		Curve->Modify();
		Curve->FloatCurve.SetKeys(Keys);
		ApplyMetadata(Curve->FloatCurve, Entry, 0, false);
		if (const FKDFNode* Node = Entry.Find(TEXT("isEventCurve")))
		{
			bool bIsEventCurve = false;
			Node->TryGetBool(bIsEventCurve);
			Curve->bIsEventCurve = bIsEventCurve;
		}
		Curve->OnCurveChanged(Curve->GetCurves());
	}

	void ApplyVectorCurve(UCurveVector* Curve, const FKDFNode& Entry, const TArray<FRichCurveKey> Keys[3])
	{
		Curve->Modify();
		for (int32 Channel = 0; Channel < 3; ++Channel)
		{
			Curve->FloatCurves[Channel].SetKeys(Keys[Channel]);
			ApplyMetadata(Curve->FloatCurves[Channel], Entry, Channel, true);
		}
		Curve->OnCurveChanged(Curve->GetCurves());
	}

	FString DescribeCurveKeyCount(const UObject* Curve, const bool bVector)
	{
		if (bVector)
		{
			const UCurveVector* VectorCurve = CastChecked<UCurveVector>(Curve);
			return FString::Printf(TEXT("%d/%d/%d keys"), VectorCurve->FloatCurves[0].GetNumKeys(),
								   VectorCurve->FloatCurves[1].GetNumKeys(), VectorCurve->FloatCurves[2].GetNumKeys());
		}
		return FString::Printf(TEXT("%d keys"), CastChecked<UCurveFloat>(Curve)->FloatCurve.GetNumKeys());
	}
} // namespace

UKDFCurveHandler::UKDFCurveHandler()
{
	mRootType = TEXT("curve");
	mStage = EKDFStage::Assets;
}

bool UKDFCurveHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	const FString CurveType = Document.GetString(TEXT("curveType"), FString()).ToLower();
	if (CurveType != TEXT("float") && CurveType != TEXT("vector"))
	{
		Context.AddError(TEXT("curve document requires curveType: float or curveType: vector"));
		return false;
	}
	const bool bVector = CurveType == TEXT("vector");
	const FKDFNode* Curves = Document.Find(TEXT("curves"));
	if (Curves == nullptr || !Curves->IsSequence() || Curves->Num() == 0)
	{
		Context.AddError(TEXT("curve document requires a non-empty 'curves' sequence"));
		return false;
	}

	bool bAnyValid = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Curves->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		if (!Entry.IsMap())
		{
			Context.AddError(TEXT("Curve entry must be a map"), Entry.Line);
			continue;
		}
		const bool bHasId = !Entry.GetString(TEXT("id"), FString()).IsEmpty();
		const bool bHasTarget = !Entry.GetString(TEXT("target"), FString()).IsEmpty();
		if (bHasId == bHasTarget)
		{
			Context.AddError(TEXT("Curve entry requires exactly one of 'id' or 'target'"), Entry.Line);
			continue;
		}

		FString Error;
		TArray<FRichCurveKey> Keys[3];
		if (!BuildCurveKeys(Entry, bVector, Keys, Error) || !ValidateCurveMetadata(Entry, bVector, Error))
		{
			Context.AddError(Error, Entry.Line);
			continue;
		}
		bAnyValid = true;
	}
	return bAnyValid;
}

bool UKDFCurveHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	UKDFSubsystem* Subsystem = UKDFSubsystem::Get(Context.mGameInstance);
	const FKDFNode* Curves = Document.Find(TEXT("curves"));
	if (Subsystem == nullptr || Curves == nullptr || !Curves->IsSequence())
	{
		return false;
	}
	const bool bVector =
		Document.GetString(TEXT("curveType"), FString()).Equals(TEXT("vector"), ESearchCase::IgnoreCase);
	UClass* CurveClass = bVector ? UCurveVector::StaticClass() : UCurveFloat::StaticClass();

	bool bAppliedAny = false;
	for (const TSharedRef<FKDFNode>& EntryRef : Curves->Sequence)
	{
		const FKDFNode& Entry = EntryRef.Get();
		FString Error;
		TArray<FRichCurveKey> Keys[3];
		if (!BuildCurveKeys(Entry, bVector, Keys, Error))
		{
			Context.AddError(Error, Entry.Line);
			continue;
		}

		const FString Id = Entry.GetString(TEXT("id"), FString());
		const FString TargetPath = Entry.GetString(TEXT("target"), FString());
		if (Id.IsEmpty() == TargetPath.IsEmpty() || !ValidateCurveMetadata(Entry, bVector, Error))
		{
			Context.AddError(Error.IsEmpty() ? TEXT("Curve entry requires exactly one of 'id' or 'target'") : Error,
							 Entry.Line);
			continue;
		}
		UObject* Target = nullptr;
		if (!Id.IsEmpty())
		{
			if (Context.bDryRun)
			{
				continue;
			}
			Target = Subsystem->GetDynamicContent().GetOrCreateAsset(Context.mPackRef, Id, CurveClass, Error);
		}
		else
		{
			Target = FSoftObjectPath(TargetPath).TryLoad();
			if (Target == nullptr && !TargetPath.Contains(TEXT("/")) && !TargetPath.Contains(TEXT(".")))
			{
				Target = Subsystem->GetDynamicContent().FindGeneratedAsset(Context.mPackRef, TargetPath);
			}
		}

		if (Target == nullptr)
		{
			Context.AddError(Error.IsEmpty() ? FString::Printf(TEXT("Curve target not found: '%s'"), *TargetPath)
											 : Error,
							 Entry.Line);
			continue;
		}
		if (!Target->IsA(CurveClass))
		{
			Context.AddError(FString::Printf(TEXT("%s is %s, expected %s"), *Target->GetPathName(),
											 *Target->GetClass()->GetName(), *CurveClass->GetName()),
							 Entry.Line);
			continue;
		}
		Subsystem->RetainObject(Target);
		if (Context.bDryRun)
		{
			continue;
		}
		const FString PreValue = DescribeCurveKeyCount(Target, bVector);

		if (bVector)
		{
			ApplyVectorCurve(CastChecked<UCurveVector>(Target), Entry, Keys);
		}
		else
		{
			ApplyFloatCurve(CastChecked<UCurveFloat>(Target), Entry, Keys[0]);
		}
		bAppliedAny = true;
		++Context.mAppliedOpCount;
		if (FKDFPatchUtil::IsDebugEnabled(Context.bDebug))
		{
			FKDFPatchUtil::LogAppliedOp(Context.mSourceFile, Target, TEXT("<curve-keys>"), TEXT("set"), PreValue,
										DescribeCurveKeyCount(Target, bVector));
		}
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = Target->GetPathName();
			OpRecord.mPropertyPath = TEXT("<curve-keys>");
			OpRecord.mOp = EKDFOp::Set;
			OpRecord.mValueText = FKDFYamlParser::EmitString(Entry).TrimStartAndEnd();
		}
		UE_LOG(LogKDataForge, Log, TEXT("Applied %s curve %s (%d keys)"), bVector ? TEXT("vector") : TEXT("float"),
			   *Target->GetPathName(), Keys[0].Num());
	}
	return bAppliedAny;
}
