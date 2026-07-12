#include "Handlers/KDFLocalizationHandler.h"

#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/TextLocalizationResource.h"
#include "KDFLogging.h"

namespace
{
	bool ValidateStructure(const FKDFNode& Document, const FKDFContextBase& Context, FString& OutNamespace,
						   const FKDFNode*& OutEntries)
	{
		OutNamespace = Document.GetString(TEXT("namespace"), FString());
		if (OutNamespace.IsEmpty())
		{
			Context.AddError(TEXT("localization document requires a non-empty 'namespace'"));
			return false;
		}
		OutEntries = Document.Find(TEXT("entries"));
		if (OutEntries == nullptr || !OutEntries->IsMap() || OutEntries->Num() == 0)
		{
			Context.AddError(TEXT("localization document requires a non-empty 'entries' map"));
			return false;
		}
		return true;
	}

	/** Culture gate is a bidirectional prefix match so "de" applies on "de-DE" and "de-DE" applies on "de". */
	bool CultureMatchesCurrentLanguage(const FString& Culture, const FString& CurrentLanguage)
	{
		return CurrentLanguage.StartsWith(Culture, ESearchCase::IgnoreCase) ||
			Culture.StartsWith(CurrentLanguage, ESearchCase::IgnoreCase);
	}
} // namespace

UKDFLocalizationHandler::UKDFLocalizationHandler()
{
	mRootType = TEXT("localization");
	mStage = EKDFStage::Localization;
}

bool UKDFLocalizationHandler::ValidateDocument(const FKDFNode& Document, FKDFValidationContext& Context)
{
	FString Namespace;
	const FKDFNode* Entries = nullptr;
	return ValidateStructure(Document, Context, Namespace, Entries);
}

bool UKDFLocalizationHandler::ApplyDocument(const FKDFNode& Document, FKDFApplyContext& Context)
{
	FString Namespace;
	const FKDFNode* Entries = nullptr;
	if (!ValidateStructure(Document, Context, Namespace, Entries))
	{
		return false;
	}

	const FString Culture = Document.GetString(TEXT("culture"), FString());
	if (!Culture.IsEmpty())
	{
		const FString CurrentLanguage = FInternationalization::Get().GetCurrentLanguage()->GetName();
		if (!CultureMatchesCurrentLanguage(Culture, CurrentLanguage))
		{
			Context.AddInfo(FString::Printf(TEXT("skipped: culture %s does not match current language %s"), *Culture,
											*CurrentLanguage));
			return false;
		}
	}

	FTextLocalizationResource Resource;
	int32 ValidCount = 0;
	int32 AppliedCount = 0;
	for (const TTuple<FString, TSharedRef<FKDFNode>>& Pair : Entries->Map)
	{
		const FString& Key = Pair.Key;
		const FKDFNode& Value = Pair.Value.Get();
		FString Source;
		FString Text;
		if (Value.IsScalar())
		{
			// Scalar form registers a new text: source string == translated text.
			Source = Value.Scalar;
			Text = Value.Scalar;
		}
		else if (Value.IsMap())
		{
			Text = Value.GetString(TEXT("text"), FString());
			if (Text.IsEmpty())
			{
				Context.AddError(FString::Printf(TEXT("Entry '%s' requires a non-empty 'text' value"), *Key),
								 Value.Line);
				continue;
			}
			// Overriding an existing text requires 'source' to match the original string exactly.
			Source = Value.GetString(TEXT("source"), Text);
		}
		else
		{
			Context.AddError(FString::Printf(TEXT("Entry '%s' must be a string or a map with 'source'/'text'"), *Key),
							 Value.Line);
			continue;
		}

		++ValidCount;
		if (Context.bDryRun)
		{
			continue;
		}

		Resource.AddEntry(FTextKey(Namespace), FTextKey(Key), Source, Text, 0);
		++AppliedCount;
		++Context.mAppliedOpCount;
		if (Context.mPatchRecord != nullptr)
		{
			FKDFOpRecord& OpRecord = Context.mPatchRecord->mOps.AddDefaulted_GetRef();
			OpRecord.mTargetObjectPath = TEXT("<Localization>");
			OpRecord.mPropertyPath = Namespace + TEXT(".") + Key;
			OpRecord.mOp = EKDFOp::Set;
			OpRecord.mValueText = Text;
		}
	}

	if (Context.bDryRun)
	{
		return ValidCount > 0;
	}
	if (AppliedCount == 0)
	{
		return false;
	}

	FTextLocalizationManager::Get().UpdateFromLocalizationResource(Resource);
	UE_LOG(LogKDataForge, Log, TEXT("Applied %d localization entr%s for namespace '%s' from %s"), AppliedCount,
		   AppliedCount == 1 ? TEXT("y") : TEXT("ies"), *Namespace, *Context.mSourceFile);
	return true;
}
