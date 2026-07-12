#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "KDFNode.h"
#include "Registry/ModContentRegistry.h"
#include "Reflection/KDFValueCodec.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKDFValueCodecBitflagEnumTest, "KDataForge.Reflection.ValueCodec.BitflagEnum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKDFValueCodecBitflagEnumTest::RunTest(const FString& Parameters)
{
	const FProperty* FlagsProperty = FindFProperty<FProperty>(FGameObjectRegistration::StaticStruct(),
		GET_MEMBER_NAME_CHECKED(FGameObjectRegistration, Flags));
	TestNotNull(TEXT("Flags property exists"), FlagsProperty);
	if (FlagsProperty == nullptr)
	{
		return false;
	}

	FGameObjectRegistration Registration;
	void* ValuePtr = FlagsProperty->ContainerPtrToValuePtr<void>(&Registration);
	FString Error;
	const TSharedRef<FKDFNode> CombinedFlags = FKDFNode::MakeScalar(TEXT("3"));
	TestTrue(TEXT("Combined declared flags import"),
		FKDFValueCodec::NodeToProperty(CombinedFlags.Get(), FlagsProperty, ValuePtr, Error));
	TestEqual(TEXT("Combined flag value"), static_cast<uint8>(Registration.Flags), uint8{3});

	Error.Reset();
	const TSharedRef<FKDFNode> UnknownFlag = FKDFNode::MakeScalar(TEXT("16"));
	TestFalse(TEXT("Unknown flag bit rejected"),
		FKDFValueCodec::NodeToProperty(UnknownFlag.Get(), FlagsProperty, ValuePtr, Error));
	TestEqual(TEXT("Rejected import leaves prior value intact"), static_cast<uint8>(Registration.Flags), uint8{3});
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
