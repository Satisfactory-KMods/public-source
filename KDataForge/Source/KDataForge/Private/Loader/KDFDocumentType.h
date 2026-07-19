#pragma once

#include "CoreMinimal.h"

namespace KDFDocumentType
{
	inline FName Normalize(const FString& TypeName)
	{
		const FString NormalizedType = TypeName.ToLower();
		return NormalizedType == TEXT("mam") ? FName(TEXT("research")) : FName(*NormalizedType);
	}
} // namespace KDFDocumentType
