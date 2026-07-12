#include "KDFTypes.h"

#include "KDataForgeApiLogging.h"

FString FKDFDiagnostic::ToString() const
{
	const TCHAR* SeverityText = TEXT("Info");
	switch (mSeverity)
	{
	case EKDFSeverity::Warning:
		SeverityText = TEXT("Warning");
		break;
	case EKDFSeverity::Error:
		SeverityText = TEXT("Error");
		break;
	default:
		break;
	}
	if (mFile.IsEmpty())
	{
		return FString::Printf(TEXT("[%s] %s"), SeverityText, *mMessage);
	}
	if (mLine == INDEX_NONE)
	{
		return FString::Printf(TEXT("[%s] %s: %s"), SeverityText, *mFile, *mMessage);
	}
	return FString::Printf(TEXT("[%s] %s:%d: %s"), SeverityText, *mFile, mLine, *mMessage);
}

void FKDFContextBase::AddDiagnostic(EKDFSeverity Severity, const FString& Message, int32 Line) const
{
	if (mDiagnostics == nullptr)
	{
		return;
	}
	FKDFDiagnostic& Diagnostic = mDiagnostics->AddDefaulted_GetRef();
	Diagnostic.mSeverity = Severity;
	Diagnostic.mMessage = Message;
	Diagnostic.mFile = mSourceFile;
	Diagnostic.mLine = Line;

	// Previously silent until `/kdf report` — a class resolve failure or a skipped document should
	// be visible in the log the moment it happens, not just on request.
	switch (Severity)
	{
	case EKDFSeverity::Error:
		UE_LOG(LogKDataForgeApi, Error, TEXT("%s"), *Diagnostic.ToString());
		break;
	case EKDFSeverity::Warning:
		UE_LOG(LogKDataForgeApi, Warning, TEXT("%s"), *Diagnostic.ToString());
		break;
	default:
		UE_LOG(LogKDataForgeApi, Display, TEXT("%s"), *Diagnostic.ToString());
		break;
	}
}

void FKDFContextBase::AddInfo(const FString& Message, int32 Line) const
{
	AddDiagnostic(EKDFSeverity::Info, Message, Line);
}

void FKDFContextBase::AddWarning(const FString& Message, int32 Line) const
{
	AddDiagnostic(EKDFSeverity::Warning, Message, Line);
}

void FKDFContextBase::AddError(const FString& Message, int32 Line) const
{
	AddDiagnostic(EKDFSeverity::Error, Message, Line);
}
