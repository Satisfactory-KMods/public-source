// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "Structures/ApiStatics.h"

#include "BFL/KBFL_ConfigTools.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

TSubclassOf<UModConfiguration> FSBSStatics::GETMODCONFIG()
{

	static TWeakObjectPtr<UClass> ConfigClass;
	if (ConfigClass.IsValid())
	{
		return ConfigClass.Get();
	}
	ConfigClass = LoadClass<UModConfiguration>(nullptr, TEXT("/SBS/SBS_ModConfig.SBS_ModConfig_C"));
	return ConfigClass.Get();
}

FString FSBSStatics::MakeUrl(FString To, UObject* WorldContext)
{
	if (const int32 Port = GetLocalTestPort())
	{
		return FString::Printf(TEXT("http://127.0.0.1:%d/api/v1/sbs/"), Port) + To;
	}
	const TSubclassOf<UModConfiguration> Config = GETMODCONFIG();
	if (IsValid(WorldContext) && Config)
	{
		if (UKBFL_ConfigTools::GetBoolFromConfig(Config, TEXT("uselocaldev"), WorldContext))
		{
			return API_URL_LOCALDEV + To;
		}
		if (UKBFL_ConfigTools::GetBoolFromConfig(Config, TEXT("usedev"), WorldContext))
		{
			return API_URL_DEV + To;
		}
	}
	return API_URL + To;
}

FString FSBSStatics::GetAccountKey(UObject* WorldContext)
{
	if (GetLocalTestPort())
	{
		return TEXT("local-fixture");
	}
	const TSubclassOf<UModConfiguration> Config = GETMODCONFIG();
	return IsValid(WorldContext) && Config
		? UKBFL_ConfigTools::GetStringFromConfig(Config, TEXT("accountkey"), WorldContext)
		: FString();
}

int32 FSBSStatics::GetLocalTestPort()
{
	static const int32 Port = []()
	{
		FString Value;
		int32 Parsed = 0;
		return FParse::Value(FCommandLine::Get(), TEXT("SBSLocalTestPort="), Value) &&
				LexTryParseString(Parsed, *Value) && Parsed >= 1024 && Parsed <= 65535
			? Parsed
			: 0;
	}();
	return Port;
}

FString FSBSStatics::EncodePathSegment(const FString& Segment) { return FGenericPlatformHttp::UrlEncode(Segment); }

bool FSBSStatics::IsSafeIdentifier(const FString& Identifier)
{
	if (Identifier.IsEmpty() || Identifier.Len() > 128)
	{
		return false;
	}
	for (const TCHAR Character : Identifier)
	{
		if (!((Character >= TEXT('a') && Character <= TEXT('z')) ||
			  (Character >= TEXT('A') && Character <= TEXT('Z')) ||
			  (Character >= TEXT('0') && Character <= TEXT('9')) || Character == TEXT('-') || Character == TEXT('_')))
		{
			return false;
		}
	}
	return true;
}

bool FSBSStatics::IsSafeFileName(const FString& Name)
{

	if (Name.IsEmpty() || Name.Len() > 180 || Name == TEXT(".") || Name == TEXT("..") || Name.EndsWith(TEXT(".")) ||
		Name.EndsWith(TEXT(" ")))
	{
		return false;
	}
	for (const TCHAR Character : Name)
	{
		if (Character < 32 || Character == 127 || FCString::Strchr(TEXT("/\\:<>\"|?*"), Character))
		{
			return false;
		}
	}
	FString Stem;
	if (!Name.Split(TEXT("."), &Stem, nullptr))
	{
		Stem = Name;
	}
	Stem.TrimEndInline();
	Stem.ToUpperInline();
	if (Stem == TEXT("CON") || Stem == TEXT("PRN") || Stem == TEXT("AUX") || Stem == TEXT("NUL") ||
		Stem == TEXT("CONIN$") || Stem == TEXT("CONOUT$"))
	{
		return false;
	}
	return !(Stem.Len() == 4 && (Stem.StartsWith(TEXT("COM")) || Stem.StartsWith(TEXT("LPT"))) &&
			 ((Stem[3] >= TEXT('0') && Stem[3] <= TEXT('9')) || Stem[3] == 0xB9 || Stem[3] == 0xB2 || Stem[3] == 0xB3));
}
