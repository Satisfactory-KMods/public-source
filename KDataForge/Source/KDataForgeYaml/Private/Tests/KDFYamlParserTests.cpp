#if WITH_DEV_AUTOMATION_TESTS

#include "KDFNode.h"
#include "KDFYamlParser.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKDFYamlQuotedKeyRoundTripTest, "KDataForge.Yaml.QuotedKeyRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKDFYamlQuotedKeyRoundTripTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FKDFNode> Root = FKDFNode::MakeMap();
	const FString Key = FString(TEXT("quoted\"key")) + FString::Chr(92) + TEXT("tail");
	Root->SetChild(Key, FKDFNode::MakeScalar(TEXT("value"), true));

	const FString Emitted = FKDFYamlParser::EmitString(Root.Get());
	FString Error;
	const TSharedPtr<FKDFNode> Parsed = FKDFYamlParser::ParseString(Emitted, Error);
	TestTrue(TEXT("Emitted YAML parses"), Parsed.IsValid());
	TestTrue(TEXT("Round-trip has no parser error"), Error.IsEmpty());
	if (Parsed.IsValid())
	{
		const FKDFNode* Value = Parsed->Find(Key);
		TestNotNull(TEXT("Quoted map key survives round-trip"), Value);
		if (Value != nullptr)
		{
			TestEqual(TEXT("Map value survives round-trip"), Value->GetString(), FString(TEXT("value")));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKDFYamlSourceLineTest, "KDataForge.Yaml.SourceLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKDFYamlSourceLineTest::RunTest(const FString& Parameters)
{
	FString Error;
	const TSharedPtr<FKDFNode> Parsed = FKDFYamlParser::ParseString(
		TEXT("type: cdo\nproperties:\n  - property: mValue\n    value: 42\n"), Error);
	TestTrue(TEXT("YAML parses"), Parsed.IsValid());
	if (!Parsed.IsValid())
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("Root starts on line 1"), Parsed->Line, 1);
	const FKDFNode* Properties = Parsed->Find(TEXT("properties"));
	TestNotNull(TEXT("Properties exists"), Properties);
	if (Properties != nullptr)
	{
		TestEqual(TEXT("Properties starts on line 2"), Properties->Line, 2);
		TestEqual(TEXT("First operation starts on line 3"), Properties->Sequence[0].Get().Line, 3);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
