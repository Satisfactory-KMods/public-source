#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Reflection/KDFPropertyPath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKDFPropertyPathEscapedKeyTest, "KDataForge.Reflection.PropertyPathEscapedKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKDFPropertyPathEscapedKeyTest::RunTest(const FString& Parameters)
{
	FKDFPropertyPath Path;
	FString Error;
	TestTrue(TEXT("Escaped map-key path parses"),
		FKDFPropertyPath::Parse(TEXT("Values[\"quoted\\\"key\\\\tail\"]"), Path, Error));
	TestTrue(TEXT("Escaped map-key path has no error"), Error.IsEmpty());
	TestEqual(TEXT("Path contains property and key segments"), Path.mSegments.Num(), 2);
	if (Path.mSegments.Num() == 2)
	{
		TestEqual(TEXT("Decoded map key matches source"), Path.mSegments[1].mKey, FString(TEXT("quoted\"key\\tail")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
