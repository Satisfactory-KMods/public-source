#include "KDFYamlParser.h"

#include <stdexcept>
#include <string>

THIRD_PARTY_INCLUDES_START
#pragma push_macro("check")
#pragma push_macro("verify")
#pragma push_macro("ensure")
#pragma push_macro("DEFAULTS")
#pragma push_macro("LOCATIONS")
#undef check
#undef verify
#undef ensure
#undef DEFAULTS
#undef LOCATIONS
#define RYML_SINGLE_HDR_DEFINE_NOW
#include "ryml/rapidyaml.hpp"
#pragma pop_macro("LOCATIONS")
#pragma pop_macro("DEFAULTS")
#pragma pop_macro("ensure")
#pragma pop_macro("verify")
#pragma pop_macro("check")
THIRD_PARTY_INCLUDES_END

namespace
{
	/** Error type thrown by the ryml error callback; caught inside this module only. */
	class FKDFYamlParseError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	[[noreturn]] void RymlErrorBasic(ryml::csubstr Msg, ryml::ErrorDataBasic const& /*ErrorData*/, void* /*UserData*/)
	{
		throw FKDFYamlParseError(std::string(Msg.str, Msg.len));
	}

	[[noreturn]] void RymlErrorParse(ryml::csubstr Msg, ryml::ErrorDataParse const& ErrorData, void* /*UserData*/)
	{
		std::string Message(Msg.str, Msg.len);
		Message += " (line " + std::to_string(ErrorData.ymlloc.line + 1) + ", col " +
			std::to_string(ErrorData.ymlloc.col + 1) + ")";
		throw FKDFYamlParseError(Message);
	}

	[[noreturn]] void RymlErrorVisit(ryml::csubstr Msg, ryml::ErrorDataVisit const& /*ErrorData*/, void* /*UserData*/)
	{
		throw FKDFYamlParseError(std::string(Msg.str, Msg.len));
	}

	const ryml::Callbacks& GetKDFCallbacks()
	{
		static const ryml::Callbacks Callbacks = []
		{
			ryml::Callbacks ConfiguredCallbacks;
			ConfiguredCallbacks.set_error_basic(&RymlErrorBasic);
			ConfiguredCallbacks.set_error_parse(&RymlErrorParse);
			ConfiguredCallbacks.set_error_visit(&RymlErrorVisit);
			return ConfiguredCallbacks;
		}();
		return Callbacks;
	}

	FString CSubstrToString(const ryml::csubstr Value)
	{
		if (Value.str == nullptr || Value.len == 0)
		{
			return FString();
		}
		const FUTF8ToTCHAR Converted(Value.str, static_cast<int32>(Value.len));
		return FString(Converted.Length(), Converted.Get());
	}

	void SetSourceLine(FKDFNode& Result, const ryml::ConstNodeRef& Node, const ryml::Parser& Parser)
	{
		const ryml::Location Location = Node.tree()->location(Parser, Node.id());
		if (Location.line != ryml::npos && Location.line < static_cast<size_t>(MAX_int32))
		{
			Result.Line = static_cast<int32>(Location.line) + 1;
		}
	}

	TSharedRef<FKDFNode> ConvertNode(const ryml::ConstNodeRef& Node, const ryml::Parser& Parser)
	{
		if (Node.is_map())
		{
			TSharedRef<FKDFNode> Result = FKDFNode::MakeMap();
			SetSourceLine(Result.Get(), Node, Parser);
			for (const ryml::ConstNodeRef& Child : Node.children())
			{
				Result->Map.Emplace(CSubstrToString(Child.key()), ConvertNode(Child, Parser));
			}
			return Result;
		}
		if (Node.is_seq())
		{
			TSharedRef<FKDFNode> Result = FKDFNode::MakeSequence();
			SetSourceLine(Result.Get(), Node, Parser);
			for (const ryml::ConstNodeRef& Child : Node.children())
			{
				Result->Sequence.Add(ConvertNode(Child, Parser));
			}
			return Result;
		}
		if (Node.has_val())
		{
			// Unquoted "null"/"~"/empty values stay Null; quoted ones are real string scalars.
			TSharedRef<FKDFNode> Result = Node.val_is_null() && !Node.is_val_quoted()
				? FKDFNode::MakeNull()
				: FKDFNode::MakeScalar(CSubstrToString(Node.val()), Node.is_val_quoted());
			SetSourceLine(Result.Get(), Node, Parser);
			return Result;
		}
		TSharedRef<FKDFNode> Result = FKDFNode::MakeNull();
		SetSourceLine(Result.Get(), Node, Parser);
		return Result;
	}
	// Number/bool lookalikes are not quoted here; FKDFNode::bQuoted, set by the value codec for string properties,
	// controls that distinction.
	bool ScalarNeedsQuotes(const FString& Value)
	{
		if (Value.IsEmpty())
		{
			return true;
		}
		if (Value.Equals(TEXT("null"), ESearchCase::IgnoreCase) || Value == TEXT("~"))
		{
			return true;
		}
		if (FChar::IsWhitespace(Value[0]) || FChar::IsWhitespace(Value[Value.Len() - 1]))
		{
			return true;
		}
		const TCHAR* Special = TEXT(":#{}[],&*!|>'\"%@`\n\t\\");
		for (const TCHAR Char : Value)
		{
			if (FCString::Strchr(Special, Char) != nullptr)
			{
				return true;
			}
		}
		return Value.StartsWith(TEXT("- ")) || Value.StartsWith(TEXT("? "));
	}

	FString EscapeDoubleQuoted(const FString& Value);

	FString EmitScalar(const FKDFNode& Node)
	{
		if (Node.IsNull())
		{
			return TEXT("null");
		}
		if (!Node.bQuoted && !ScalarNeedsQuotes(Node.Scalar))
		{
			return Node.Scalar;
		}
		return FString::Printf(TEXT("\"%s\""), *EscapeDoubleQuoted(Node.Scalar));
	}

	FString EscapeDoubleQuoted(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
		Escaped.ReplaceInline(TEXT("\r"), TEXT("\\r"));
		Escaped.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		const TCHAR TabString[] = {TEXT('	'), TEXT('\0')};
		const TCHAR EscapedTabString[] = {TEXT('\\'), TEXT('t'), TEXT('\0')};
		Escaped.ReplaceInline(TabString, EscapedTabString);
		return Escaped;
	}

	FString EmitKey(const FString& Key)
	{
		return ScalarNeedsQuotes(Key) ? FString::Printf(TEXT("\"%s\""), *EscapeDoubleQuoted(Key)) : Key;
	}

	void EmitNode(const FKDFNode& Node, FString& Out, int32 Indent)
	{
		const FString Pad = FString::ChrN(Indent * 2, TEXT(' '));
		switch (Node.Type)
		{
		case EKDFNodeType::Map:
			{
				if (Node.Map.IsEmpty())
				{
					Out += Pad + TEXT("{}\n");
					break;
				}
				for (const TTuple<FString, TSharedRef<FKDFNode>>& Pair : Node.Map)
				{
					const FKDFNode& Child = Pair.Value.Get();
					if (Child.IsScalar() || Child.IsNull() || Child.Num() == 0)
					{
						FString Inline;
						if (Child.IsScalar() || Child.IsNull())
						{
							Inline = EmitScalar(Child);
						}
						else
						{
							Inline = Child.IsMap() ? TEXT("{}") : TEXT("[]");
						}
						Out += FString::Printf(TEXT("%s%s: %s\n"), *Pad, *EmitKey(Pair.Key), *Inline);
					}
					else
					{
						Out += FString::Printf(TEXT("%s%s:\n"), *Pad, *EmitKey(Pair.Key));
						EmitNode(Child, Out, Indent + 1);
					}
				}
				break;
			}
		case EKDFNodeType::Sequence:
			{
				if (Node.Sequence.IsEmpty())
				{
					Out += Pad + TEXT("[]\n");
					break;
				}
				for (const TSharedRef<FKDFNode>& ChildRef : Node.Sequence)
				{
					const FKDFNode& Child = ChildRef.Get();
					if (Child.IsScalar() || Child.IsNull())
					{
						Out += FString::Printf(TEXT("%s- %s\n"), *Pad, *EmitScalar(Child));
					}
					else if (Child.Num() == 0)
					{
						Out += FString::Printf(TEXT("%s- %s\n"), *Pad, Child.IsMap() ? TEXT("{}") : TEXT("[]"));
					}
					else
					{
						// Emit the child block and splice "- " into its first line.
						FString ChildText;
						EmitNode(Child, ChildText, Indent + 1);
						const FString ChildPad = FString::ChrN((Indent + 1) * 2, TEXT(' '));
						ChildText.RemoveFromStart(ChildPad);
						Out += FString::Printf(TEXT("%s- %s"), *Pad, *ChildText);
					}
				}
				break;
			}
		default:
			Out += Pad + EmitScalar(Node) + TEXT("\n");
			break;
		}
	}
} // namespace

TSharedPtr<FKDFNode> FKDFYamlParser::ParseString(const FString& YamlText, FString& OutError)
{
	OutError.Reset();
	const FTCHARToUTF8 Utf8(*YamlText);
	try
	{
		const ryml::csubstr Source(Utf8.Get(), static_cast<size_t>(Utf8.Length()));
		ryml::Tree Tree(GetKDFCallbacks());
		ryml::EventHandlerTree EventHandler(GetKDFCallbacks());
		ryml::ParserOptions ParserOptions;
		ParserOptions.locations(true);
		ryml::Parser Parser(&EventHandler, ParserOptions);
		ryml::parse_in_arena(&Parser, Source, &Tree);
		ryml::ConstNodeRef Root = Tree.rootref();
		if (Root.is_stream())
		{
			if (Root.num_children() == 0)
			{
				OutError = TEXT("Document stream is empty");
				return nullptr;
			}
			Root = Root.first_child();
		}
		return ConvertNode(Root, Parser);
	}
	catch (const std::exception& Exception)
	{
		OutError = UTF8_TO_TCHAR(Exception.what());
		return nullptr;
	}
}

TArray<TSharedPtr<FKDFNode>> FKDFYamlParser::ParseStringMulti(const FString& YamlText, FString& OutError)
{
	OutError.Reset();
	TArray<TSharedPtr<FKDFNode>> OutDocuments;
	const FTCHARToUTF8 Utf8(*YamlText);
	try
	{
		const ryml::csubstr Source(Utf8.Get(), static_cast<size_t>(Utf8.Length()));
		ryml::Tree Tree(GetKDFCallbacks());
		ryml::EventHandlerTree EventHandler(GetKDFCallbacks());
		ryml::ParserOptions ParserOptions;
		ParserOptions.locations(true);
		ryml::Parser Parser(&EventHandler, ParserOptions);
		ryml::parse_in_arena(&Parser, Source, &Tree);
		const ryml::ConstNodeRef Root = Tree.rootref();
		if (Root.is_stream())
		{
			for (const ryml::ConstNodeRef& Doc : Root.children())
			{
				OutDocuments.Add(ConvertNode(Doc, Parser));
			}
		}
		else
		{
			OutDocuments.Add(ConvertNode(Root, Parser));
		}
	}
	catch (const std::exception& Exception)
	{
		OutError = UTF8_TO_TCHAR(Exception.what());
		OutDocuments.Reset();
	}
	return OutDocuments;
}

FString FKDFYamlParser::EmitString(const FKDFNode& Root)
{
	FString Result;
	EmitNode(Root, Result, 0);
	return Result;
}

void KDFYamlInstallErrorCallbacks() { ryml::set_callbacks(GetKDFCallbacks()); }
