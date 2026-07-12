#pragma once

#include "CoreMinimal.h"

#include "KDFNode.h"

/**
 * Thin façade over the rapidyaml backend. The only YAML entry point in KDataForge —
 * backend types never cross this module boundary.
 */
class KDATAFORGEYAML_API FKDFYamlParser
{
public:
	/**
	 * Parses YAML text into an FKDFNode tree.
	 * Multi-document streams return the first document only — use ParseStringMulti to see every
	 * document in a `---`-separated stream.
	 * Thread-safe: may be called from worker threads (used by the parallel loader).
	 *
	 * @return Root node, or nullptr with OutError set on malformed input.
	 */
	static TSharedPtr<FKDFNode> ParseString(const FString& YamlText, FString& OutError);

	/**
	 * Parses YAML text as a stream of one or more `---`-separated documents (Kubernetes-manifest
	 * style). A plain single-document file (no `---` stream marker) still returns exactly one entry,
	 * so this is a strict superset of ParseString and safe to use unconditionally.
	 * Thread-safe: may be called from worker threads (used by the parallel loader).
	 *
	 * @return One entry per document in source order, or an empty array with OutError set on
	 *         malformed input. Individual documents may still be null nodes (e.g. an empty document
	 *         between two `---` separators) — callers decide whether that's meaningful.
	 */
	static TArray<TSharedPtr<FKDFNode>> ParseStringMulti(const FString& YamlText, FString& OutError);

	/** Emits an FKDFNode tree as YAML text (used by exporters). */
	static FString EmitString(const FKDFNode& Root);
};
