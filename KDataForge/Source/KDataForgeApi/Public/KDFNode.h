// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EKDFNodeType : uint8
{
	Null,
	Scalar,
	Sequence,
	Map
};

struct KDATAFORGEAPI_API FKDFNode
{
	EKDFNodeType Type = EKDFNodeType::Null;

	FString Scalar;

	bool bQuoted = false;

	int32 Line = INDEX_NONE;

	TArray<TSharedRef<FKDFNode>> Sequence;

	TArray<TTuple<FString, TSharedRef<FKDFNode>>> Map;

	FORCEINLINE bool IsNull() const { return Type == EKDFNodeType::Null; }

	FORCEINLINE bool IsScalar() const { return Type == EKDFNodeType::Scalar; }

	FORCEINLINE bool IsSequence() const { return Type == EKDFNodeType::Sequence; }

	FORCEINLINE bool IsMap() const { return Type == EKDFNodeType::Map; }

	const FKDFNode* Find(const FString& Key) const;

	TSharedPtr<FKDFNode> FindShared(const FString& Key) const;

	int32 Num() const;

	bool TryGetBool(bool& OutValue) const;
	bool TryGetInt(int64& OutValue) const;
	bool TryGetFloat(double& OutValue) const;

	FString GetString(const FString& Default = FString()) const;

	FString GetString(const FString& Key, const FString& Default) const;
	bool GetBool(const FString& Key, bool bDefault) const;
	int64 GetInt(const FString& Key, int64 Default) const;
	double GetFloat(const FString& Key, double Default) const;

	static TSharedRef<FKDFNode> MakeNull();
	static TSharedRef<FKDFNode> MakeScalar(const FString& Value, bool bInQuoted = false);
	static TSharedRef<FKDFNode> MakeSequence();
	static TSharedRef<FKDFNode> MakeMap();

	void SetChild(const FString& Key, const TSharedRef<FKDFNode>& Child);

	void AddChild(const TSharedRef<FKDFNode>& Child);
};
