#pragma once

#include "CoreMinimal.h"

/** Node kind of a parsed YAML value. */
enum class EKDFNodeType : uint8
{
	Null,
	Scalar,
	Sequence,
	Map
};

/**
 * Engine-side value model for a parsed YAML document.
 * KDataForge handlers only ever see this type — the YAML parser backend (rapidyaml) never crosses a module boundary.
 * Maps preserve insertion order so documents apply deterministically.
 */
struct KDATAFORGEAPI_API FKDFNode
{
	EKDFNodeType Type = EKDFNodeType::Null;

	/** Scalar payload, valid when Type == Scalar. */
	FString Scalar;

	/** True when the scalar was quoted in the source (forces string interpretation). */
	bool bQuoted = false;

	/** Source line (1-based) if the parser backend provided it, INDEX_NONE otherwise. */
	int32 Line = INDEX_NONE;

	/** Children, valid when Type == Sequence. */
	TArray<TSharedRef<FKDFNode>> Sequence;

	/** Key → child pairs in document order, valid when Type == Map. */
	TArray<TTuple<FString, TSharedRef<FKDFNode>>> Map;

	FORCEINLINE bool IsNull() const { return Type == EKDFNodeType::Null; }

	FORCEINLINE bool IsScalar() const { return Type == EKDFNodeType::Scalar; }

	FORCEINLINE bool IsSequence() const { return Type == EKDFNodeType::Sequence; }

	FORCEINLINE bool IsMap() const { return Type == EKDFNodeType::Map; }

	/** Finds a direct child of a map node by key. Returns nullptr for missing keys or non-map nodes. */
	const FKDFNode* Find(const FString& Key) const;

	/** Same as Find but returns the shared node (for holders that must keep the subtree alive). */
	TSharedPtr<FKDFNode> FindShared(const FString& Key) const;

	/** Number of children (sequence elements or map pairs). */
	int32 Num() const;

	// --- Scalar accessors (valid on scalar nodes) ---

	bool TryGetBool(bool& OutValue) const;
	bool TryGetInt(int64& OutValue) const;
	bool TryGetFloat(double& OutValue) const;

	FString GetString(const FString& Default = FString()) const;

	// --- Map convenience accessors ---

	FString GetString(const FString& Key, const FString& Default) const;
	bool GetBool(const FString& Key, bool bDefault) const;
	int64 GetInt(const FString& Key, int64 Default) const;
	double GetFloat(const FString& Key, double Default) const;

	// --- Builders (used by exporters) ---

	static TSharedRef<FKDFNode> MakeNull();
	static TSharedRef<FKDFNode> MakeScalar(const FString& Value, bool bInQuoted = false);
	static TSharedRef<FKDFNode> MakeSequence();
	static TSharedRef<FKDFNode> MakeMap();

	/** Adds or replaces a map entry. Asserts on non-map nodes. */
	void SetChild(const FString& Key, const TSharedRef<FKDFNode>& Child);

	/** Appends a sequence element. Asserts on non-sequence nodes. */
	void AddChild(const TSharedRef<FKDFNode>& Child);
};
