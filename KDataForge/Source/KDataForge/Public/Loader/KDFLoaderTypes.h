// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "KDFNode.h"

enum class EKDFContentRegKind : uint8
{
	None,
	Recipe,
	Schematic,
	ResearchTree
};

struct KDATAFORGE_API FKDFPack
{

	FString mRef;

	FString mName;
	FString mVersion;

	int32 mPriority = 100;

	bool bEnabled = true;

	bool bDebug = false;

	FString mDirectory;

	TSharedPtr<FKDFNode> mConditionsNode;

	TSharedPtr<FKDFNode> mConditionBehaviorNode;

	TArray<FString> mDependencies;

	TMap<FString, FString> mRedirects;
};

struct KDATAFORGE_API FKDFDocument
{
	FString mAbsoluteFile;

	FString mRelativeFile;

	FString mPackRef;
	int32 mPackPriority = 100;

	int32 mPackOrder = INDEX_NONE;

	FName mRootType;

	TSharedPtr<FKDFNode> mRoot;

	TArray<FString> mIncludeDependencies;

	bool bDebug = false;
};

struct KDATAFORGE_API FKDFActorPatch
{
	TWeakObjectPtr<UClass> mTargetClass;

	bool bIncludeSubclasses = true;

	TSharedPtr<FKDFNode> mPropertiesNode;

	FString mSourceFile;

	FString mPackRef;
};

struct KDATAFORGE_API FKDFNodePurge
{
	TWeakObjectPtr<UClass> mResourceClass;

	TArray<uint8> mNodeTypes;

	bool bAllowOccupied = false;

	bool bRemoveFromScanner = true;

	FString mSourceFile;

	FString mPackRef;
};

struct KDATAFORGE_API FKDFContentRemoval
{
	TWeakObjectPtr<UClass> mContentClass;

	EKDFContentRegKind mKind = EKDFContentRegKind::None;

	FString mSourceFile;

	FString mPackRef;
};

struct KDATAFORGE_API FKDFLazyClassWatch
{

	TWeakObjectPtr<UClass> mBaseClass;

	FString mExactTargetPath;

	TSharedPtr<FKDFNode> mPropertiesNode;

	TArray<FGameplayTag> mMatchTags;

	FString mTagPropertyName;
	FString mSourceFile;
	FString mPackRef;
	bool bDebug = false;

	TSet<TWeakObjectPtr<UClass>> mAppliedClasses;
};
