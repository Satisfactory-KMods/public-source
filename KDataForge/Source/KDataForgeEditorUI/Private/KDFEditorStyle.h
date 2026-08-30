#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"

namespace KDFEditorStyle
{

	inline const FLinearColor Orange = FLinearColor::FromSRGBColor(FColor(0xFA, 0x95, 0x49));
	inline const FLinearColor PanelDark = FLinearColor::FromSRGBColor(FColor(0x20, 0x21, 0x25));
	inline const FLinearColor PanelMid = FLinearColor::FromSRGBColor(FColor(0x2B, 0x2C, 0x31));
	inline const FLinearColor PanelLight = FLinearColor::FromSRGBColor(FColor(0x37, 0x38, 0x3E));
	inline const FLinearColor TextBright = FLinearColor::FromSRGBColor(FColor(0xE8, 0xE8, 0xE8));
	inline const FLinearColor TextDim = FLinearColor::FromSRGBColor(FColor(0x9A, 0x9B, 0xA1));
	inline const FLinearColor ButtonPrimary = FLinearColor::FromSRGBColor(FColor(0xA6, 0x4D, 0x14));
	inline const FLinearColor ButtonSecondary = FLinearColor::FromSRGBColor(FColor(0x45, 0x4B, 0x57));
	inline const FLinearColor ButtonSuccess = FLinearColor::FromSRGBColor(FColor(0x26, 0x7A, 0x3B));
	inline const FLinearColor ButtonDanger = FLinearColor::FromSRGBColor(FColor(0xA5, 0x2A, 0x2A));
	inline const FLinearColor ButtonText = FLinearColor::White;
	inline const FLinearColor ModifiedTint = FLinearColor::FromSRGBColor(FColor(0xFA, 0x95, 0x49, 0x30));
	inline const FLinearColor ErrorRed = FLinearColor::FromSRGBColor(FColor(0xE5, 0x4B, 0x4B));
	inline const FLinearColor OkGreen = FLinearColor::FromSRGBColor(FColor(0x5F, 0xC9, 0x6E));

	constexpr float PadS = 4.f;
	constexpr float PadM = 8.f;
	constexpr float PadL = 12.f;
}
