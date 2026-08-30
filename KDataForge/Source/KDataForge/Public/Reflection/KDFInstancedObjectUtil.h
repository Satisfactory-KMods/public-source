#pragma once

#include "CoreMinimal.h"

struct FKDFNode;
struct FKDFApplyContext;

struct KDATAFORGE_API FKDFInstancedObjectUtil
{
	static bool ApplyList(UObject* Target, const FKDFNode& EntriesNode, const TCHAR* ArrayField,
						  const TCHAR* NotFoundMessage, FKDFApplyContext& Context);
};
