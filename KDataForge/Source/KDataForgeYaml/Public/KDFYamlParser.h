#pragma once

#include "CoreMinimal.h"

#include "KDFNode.h"

class KDATAFORGEYAML_API FKDFYamlParser
{
public:

	static TSharedPtr<FKDFNode> ParseString(const FString& YamlText, FString& OutError);

	static TArray<TSharedPtr<FKDFNode>> ParseStringMulti(const FString& YamlText, FString& OutError);

	static FString EmitString(const FKDFNode& Root);
};
