// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "Algo/Sort.h"
#include "Algo/StableSort.h"
#include "CoreMinimal.h"

#include "KBFLWorldModule.h"

#include "KPCLWorldModule.generated.h"

#define KPCL_CLEAR_INVALID_OBJECT_PTRS(Array)                                                                          \
	(Array).RemoveAll([](const auto& Object) { return GetValid(Object) == nullptr; })

#define KPCL_CLEAR_INVALID_OBJECT_PTRS_SWAP(Array)                                                                     \
	(Array).RemoveAllSwap([](const auto& Object) { return GetValid(Object) == nullptr; }, EAllowShrinking::No)

#define KPCL_ADD_UNIQUE_VALID_OBJECT_PTR(Array, Object)                                                                \
	do                                                                                                                 \
	{                                                                                                                  \
		if (auto* KPCLValidObject = GetValid((Object)))                                                                \
		{                                                                                                              \
			(Array).AddUnique(KPCLValidObject);                                                                        \
		}                                                                                                              \
	}                                                                                                                  \
	while (false)

#define KPCL_REMOVE_OBJECT_PTR_AND_INVALIDS(Array, Object)                                                             \
	(Array).RemoveAll([KPCLTargetObject = GetValid((Object))](const auto& Entry)                                       \
					  { return GetValid(Entry) == nullptr || Entry.Get() == KPCLTargetObject; })

#define KPCL_REMOVE_INVALID_OR_OBJECT_PTRS_IF(Array, Predicate)                                                        \
	(Array).RemoveAll(                                                                                                 \
		[&](const auto& Entry)                                                                                         \
		{                                                                                                              \
			auto* KPCLValidObject = GetValid(Entry);                                                                   \
			return KPCLValidObject == nullptr || (Predicate)(KPCLValidObject);                                         \
		})

#define KPCL_REMOVE_INVALID_OR_OBJECT_PTRS_IF_SWAP(Array, Predicate)                                                   \
	(Array).RemoveAllSwap(                                                                                             \
		[&](const auto& Entry)                                                                                         \
		{                                                                                                              \
			auto* KPCLValidObject = GetValid(Entry);                                                                   \
			return KPCLValidObject == nullptr || (Predicate)(KPCLValidObject);                                         \
		},                                                                                                             \
		EAllowShrinking::No)

#define KPCL_KEEP_VALID_OBJECT_PTRS_IF(Array, Predicate)                                                               \
	(Array).RemoveAll(                                                                                                 \
		[&](const auto& Entry)                                                                                         \
		{                                                                                                              \
			auto* KPCLValidObject = GetValid(Entry);                                                                   \
			return KPCLValidObject == nullptr || !(Predicate)(KPCLValidObject);                                        \
		})

#define KPCL_KEEP_VALID_OBJECT_PTRS_IF_SWAP(Array, Predicate)                                                          \
	(Array).RemoveAllSwap(                                                                                             \
		[&](const auto& Entry)                                                                                         \
		{                                                                                                              \
			auto* KPCLValidObject = GetValid(Entry);                                                                   \
			return KPCLValidObject == nullptr || !(Predicate)(KPCLValidObject);                                        \
		},                                                                                                             \
		EAllowShrinking::No)

#define KPCL_FOR_EACH_VALID_OBJECT_PTR(Array, Callback)                                                                \
	do                                                                                                                 \
	{                                                                                                                  \
		for (const auto& KPCLObjectPtr : (Array))                                                                      \
		{                                                                                                              \
			if (auto* KPCLValidObject = GetValid(KPCLObjectPtr))                                                       \
			{                                                                                                          \
				(Callback)(KPCLValidObject);                                                                           \
			}                                                                                                          \
		}                                                                                                              \
	}                                                                                                                  \
	while (false)

#define KPCL_COUNT_VALID_OBJECT_PTRS(Array)                                                                            \
	(                                                                                                                  \
		[&]()                                                                                                          \
		{                                                                                                              \
			int32 KPCLValidCount = 0;                                                                                  \
			for (const auto& KPCLObjectPtr : (Array))                                                                  \
			{                                                                                                          \
				KPCLValidCount += GetValid(KPCLObjectPtr) != nullptr;                                                  \
			}                                                                                                          \
			return KPCLValidCount;                                                                                     \
		}())

#define KPCL_COUNT_VALID_OBJECT_PTRS_IF(Array, Predicate)                                                              \
	(                                                                                                                  \
		[&]()                                                                                                          \
		{                                                                                                              \
			int32 KPCLValidCount = 0;                                                                                  \
			for (const auto& KPCLObjectPtr : (Array))                                                                  \
			{                                                                                                          \
				if (auto* KPCLValidObject = GetValid(KPCLObjectPtr))                                                   \
				{                                                                                                      \
					KPCLValidCount += (Predicate)(KPCLValidObject);                                                    \
				}                                                                                                      \
			}                                                                                                          \
			return KPCLValidCount;                                                                                     \
		}())

#define KPCL_HAS_VALID_OBJECT_PTRS(Array)                                                                              \
	(Array).ContainsByPredicate([](const auto& Entry) { return GetValid(Entry) != nullptr; })

#define KPCL_ANY_VALID_OBJECT_PTRS_IF(Array, Predicate)                                                                \
	(Array).ContainsByPredicate(                                                                                       \
		[&](const auto& Entry)                                                                                         \
		{                                                                                                              \
			auto* KPCLValidObject = GetValid(Entry);                                                                   \
			return KPCLValidObject != nullptr && (Predicate)(KPCLValidObject);                                         \
		})

#define KPCL_ALL_VALID_OBJECT_PTRS_IF(Array, Predicate)                                                                \
	(                                                                                                                  \
		[&]()                                                                                                          \
		{                                                                                                              \
			for (const auto& KPCLObjectPtr : (Array))                                                                  \
			{                                                                                                          \
				if (auto* KPCLValidObject = GetValid(KPCLObjectPtr))                                                   \
				{                                                                                                      \
					if (!(Predicate)(KPCLValidObject))                                                                 \
					{                                                                                                  \
						return false;                                                                                  \
					}                                                                                                  \
				}                                                                                                      \
			}                                                                                                          \
			return true;                                                                                               \
		}())

#define KPCL_NONE_VALID_OBJECT_PTRS_IF(Array, Predicate)                                                               \
	(!(Array).ContainsByPredicate(                                                                                     \
		[&](const auto& Entry)                                                                                         \
		{                                                                                                              \
			auto* KPCLValidObject = GetValid(Entry);                                                                   \
			return KPCLValidObject != nullptr && (Predicate)(KPCLValidObject);                                         \
		}))

#define KPCL_CONTAINS_VALID_OBJECT_PTR(Array, Object)                                                                  \
	(                                                                                                                  \
		[&]()                                                                                                          \
		{                                                                                                              \
			auto* KPCLTargetObject = GetValid((Object));                                                               \
			return KPCLTargetObject != nullptr &&                                                                      \
				(Array).ContainsByPredicate([KPCLTargetObject](const auto& Entry)                                      \
											{ return GetValid(Entry) == KPCLTargetObject; });                          \
		}())

#define KPCL_GET_VALID_OBJECT_PTR_AT(Array, Index)                                                                     \
	(                                                                                                                  \
		[&]() -> decltype(GetValid((Array)[0]))                                                                        \
		{                                                                                                              \
			const int32 KPCLObjectIndex = (Index);                                                                     \
			return (Array).IsValidIndex(KPCLObjectIndex) ? GetValid((Array)[KPCLObjectIndex]) : nullptr;               \
		}())

#define KPCL_FIND_FIRST_VALID_OBJECT_PTR(Array)                                                                        \
	(                                                                                                                  \
		[&]() -> decltype(GetValid((Array)[0]))                                                                        \
		{                                                                                                              \
			for (const auto& KPCLObjectPtr : (Array))                                                                  \
			{                                                                                                          \
				if (auto* KPCLValidObject = GetValid(KPCLObjectPtr))                                                   \
				{                                                                                                      \
					return KPCLValidObject;                                                                            \
				}                                                                                                      \
			}                                                                                                          \
			return nullptr;                                                                                            \
		}())

#define KPCL_FIND_LAST_VALID_OBJECT_PTR(Array)                                                                         \
	(                                                                                                                  \
		[&]() -> decltype(GetValid((Array)[0]))                                                                        \
		{                                                                                                              \
			const auto& KPCLObjectArray = (Array);                                                                     \
			for (int32 KPCLObjectIndex = KPCLObjectArray.Num() - 1; KPCLObjectIndex >= 0; --KPCLObjectIndex)           \
			{                                                                                                          \
				if (auto* KPCLValidObject = GetValid(KPCLObjectArray[KPCLObjectIndex]))                                \
				{                                                                                                      \
					return KPCLValidObject;                                                                            \
				}                                                                                                      \
			}                                                                                                          \
			return nullptr;                                                                                            \
		}())

#define KPCL_FIND_FIRST_VALID_OBJECT_PTR_IF(Array, Predicate)                                                          \
	(                                                                                                                  \
		[&]() -> decltype(GetValid((Array)[0]))                                                                        \
		{                                                                                                              \
			for (const auto& KPCLObjectPtr : (Array))                                                                  \
			{                                                                                                          \
				if (auto* KPCLValidObject = GetValid(KPCLObjectPtr))                                                   \
				{                                                                                                      \
					if ((Predicate)(KPCLValidObject))                                                                  \
					{                                                                                                  \
						return KPCLValidObject;                                                                        \
					}                                                                                                  \
				}                                                                                                      \
			}                                                                                                          \
			return nullptr;                                                                                            \
		}())

#define KPCL_INDEX_OF_VALID_OBJECT_PTR(Array, Object)                                                                  \
	(                                                                                                                  \
		[&]()                                                                                                          \
		{                                                                                                              \
			auto* KPCLTargetObject = GetValid((Object));                                                               \
			return KPCLTargetObject ? (Array).IndexOfByPredicate([KPCLTargetObject](const auto& Entry)                 \
																 { return GetValid(Entry) == KPCLTargetObject; })      \
									: INDEX_NONE;                                                                      \
		}())

#define KPCL_APPEND_VALID_OBJECT_PTRS(Target, Source)                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		const int32 KPCLSourceCount = (Source).Num();                                                                  \
		for (int32 KPCLSourceIndex = 0; KPCLSourceIndex < KPCLSourceCount; ++KPCLSourceIndex)                          \
		{                                                                                                              \
			if (auto* KPCLValidObject = GetValid((Source)[KPCLSourceIndex]))                                           \
			{                                                                                                          \
				(Target).Add(KPCLValidObject);                                                                         \
			}                                                                                                          \
		}                                                                                                              \
	}                                                                                                                  \
	while (false)

#define KPCL_APPEND_UNIQUE_VALID_OBJECT_PTRS(Target, Source)                                                           \
	do                                                                                                                 \
	{                                                                                                                  \
		const int32 KPCLSourceCount = (Source).Num();                                                                  \
		for (int32 KPCLSourceIndex = 0; KPCLSourceIndex < KPCLSourceCount; ++KPCLSourceIndex)                          \
		{                                                                                                              \
			if (auto* KPCLValidObject = GetValid((Source)[KPCLSourceIndex]))                                           \
			{                                                                                                          \
				(Target).AddUnique(KPCLValidObject);                                                                   \
			}                                                                                                          \
		}                                                                                                              \
	}                                                                                                                  \
	while (false)

#define KPCL_COPY_VALID_OBJECT_PTRS(Target, Source)                                                                    \
	do                                                                                                                 \
	{                                                                                                                  \
		(Target).Reset();                                                                                              \
		KPCL_APPEND_VALID_OBJECT_PTRS((Target), (Source));                                                             \
	}                                                                                                                  \
	while (false)

#define KPCL_SORT_VALID_OBJECT_PTRS(Array, Comparator)                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		KPCL_CLEAR_INVALID_OBJECT_PTRS(Array);                                                                         \
		Algo::Sort((Array), [&](const auto& Left, const auto& Right)                                                   \
				   { return (Comparator)(GetValid(Left), GetValid(Right)); });                                         \
	}                                                                                                                  \
	while (false)

#define KPCL_STABLE_SORT_VALID_OBJECT_PTRS(Array, Comparator)                                                          \
	do                                                                                                                 \
	{                                                                                                                  \
		KPCL_CLEAR_INVALID_OBJECT_PTRS(Array);                                                                         \
		Algo::StableSort((Array), [&](const auto& Left, const auto& Right)                                             \
						 { return (Comparator)(GetValid(Left), GetValid(Right)); });                                   \
	}                                                                                                                  \
	while (false)

UCLASS(Blueprintable)
class KPRIVATECODELIB_API UKPCLWorldModule : public UKBFLWorldModule
{
	GENERATED_BODY()

public:
	UKPCLWorldModule();

	virtual void InitPhase_Implementation() override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Options")
	bool IsEasyNodesEnabled();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Options")
	void ApplyEasyNodes(const TArray<TSubclassOf<UFGResearchTree>>& Nodes);

	UPROPERTY(EditDefaultsOnly, Category = "KMods|ADA")
	bool mUseEasyNodes = true;
};
