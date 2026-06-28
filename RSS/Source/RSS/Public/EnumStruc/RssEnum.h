// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "RssEnum.generated.h"

// Old STUFF!
UENUM(BlueprintType)
enum class ESignType : uint8
{
	RSS_InValid = 0 UMETA(DisplayName = "InValid"),
	RSS_Normal = 1 UMETA(DisplayName = "Normal"),
	RSS_Hologram = 2 UMETA(DisplayName = "Hologram"),
	RSS_Secret = 3 UMETA(DisplayName = "Secret"),
	RSS_Decal = 4 UMETA(DisplayName = "Decal")
};

UENUM(BlueprintType)
enum class EJust : uint8
{
	RSS_Left = 0 UMETA(DisplayName = "Left"),
	RSS_Middle = 1 UMETA(DisplayName = "Middle"),
	RSS_Right = 2 UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class ERssEffectType : uint8
{
	RSS_Panner = 0 UMETA(DisplayName = "Panner"),
	RSS_AnimatedImage = 1 UMETA(DisplayName = "Animated")
};

UENUM(BlueprintType)
enum class ERssTextType : uint8
{
	NormalText = 0 UMETA(DisplayName = "Normal"),
	BackgroundText = 1 UMETA(DisplayName = "Background")
};

UENUM(BlueprintType)
enum class ERssPannerType : uint8
{
	RSS_Linear = 0 UMETA(DisplayName = "Linear"),
	RSS_Step = 1 UMETA(DisplayName = "Step")
};

UENUM(BlueprintType)
enum class ESignSize : uint8
{
	RSS_InValid = 0 UMETA(DisplayName = "InValid"),
	RSS_05x05 = 1 UMETA(DisplayName = "05x05"),
	RSS_1x1 = 2 UMETA(DisplayName = "1x1"),
	RSS_1x2 = 3 UMETA(DisplayName = "1x2"),
	RSS_1x7 = 4 UMETA(DisplayName = "1x7"),
	RSS_2x05 = 5 UMETA(DisplayName = "2x05"),
	RSS_2x1 = 6 UMETA(DisplayName = "2x1"),
	RSS_2x2 = 7 UMETA(DisplayName = "2x2"),
	RSS_2x3 = 8 UMETA(DisplayName = "2x3"),
	RSS_3x05 = 9 UMETA(DisplayName = "3x05"),
	RSS_3x1 = 10 UMETA(DisplayName = "3x1"),
	RSS_4x05 = 11 UMETA(DisplayName = "4x05"),
	RSS_4x1 = 12 UMETA(DisplayName = "4x1"),
	RSS_7x1 = 13 UMETA(DisplayName = "7x1"),
	RSS_8x4 = 14 UMETA(DisplayName = "8x4"),
	RSS_16x8 = 15 UMETA(DisplayName = "16x8"),
	RSS_25x2 = 16 UMETA(DisplayName = "25x2"),
	RSS_Custom = 17 UMETA(DisplayName = "Custom"),
	RSS_Secret = 18 UMETA(DisplayName = "Secret")
};

UENUM(BlueprintType)
enum class ESignAttach : uint8
{
	RSS_Floor = 0 UMETA(DisplayName = "Floor"),
	RSS_Ceiling = 1 UMETA(DisplayName = "Ceiling"),
	RSS_Wall = 2 UMETA(DisplayName = "Wall"),
	RSS_Pole = 3 UMETA(DisplayName = "Pole")
};

// New Suff

UENUM(BlueprintType)
enum class ESignElementType : uint8
{
	Text = 0 UMETA(DisplayName = "Text"),
	Effect = 1 UMETA(DisplayName = "Effect"),
	Image = 2 UMETA(DisplayName = "Image")
};

UENUM(BlueprintType)
enum class ERssTargetActorType : uint8
{
	Invalid = 0 UMETA(DisplayName = "Invalid"),
	Belt = 1 UMETA(DisplayName = "Belt"),
	Pipe = 2 UMETA(DisplayName = "Pipe"),
	SignPole = 3 UMETA(DisplayName = "SignPole")
};
