#include "KDFNode.h"

const FKDFNode* FKDFNode::Find(const FString& Key) const
{
	if (Type != EKDFNodeType::Map)
	{
		return nullptr;
	}
	for (const TTuple<FString, TSharedRef<FKDFNode>>& Pair : Map)
	{
		if (Pair.Key == Key)
		{
			return &Pair.Value.Get();
		}
	}
	return nullptr;
}

TSharedPtr<FKDFNode> FKDFNode::FindShared(const FString& Key) const
{
	if (Type != EKDFNodeType::Map)
	{
		return nullptr;
	}
	for (const TTuple<FString, TSharedRef<FKDFNode>>& Pair : Map)
	{
		if (Pair.Key == Key)
		{
			return Pair.Value;
		}
	}
	return nullptr;
}

int32 FKDFNode::Num() const
{
	switch (Type)
	{
	case EKDFNodeType::Sequence:
		return Sequence.Num();
	case EKDFNodeType::Map:
		return Map.Num();
	default:
		return 0;
	}
}

bool FKDFNode::TryGetBool(bool& OutValue) const
{
	if (Type != EKDFNodeType::Scalar)
	{
		return false;
	}
	if (Scalar.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Scalar.Equals(TEXT("yes"), ESearchCase::IgnoreCase) ||
		Scalar.Equals(TEXT("on"), ESearchCase::IgnoreCase) || Scalar == TEXT("1"))
	{
		OutValue = true;
		return true;
	}
	if (Scalar.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Scalar.Equals(TEXT("no"), ESearchCase::IgnoreCase) ||
		Scalar.Equals(TEXT("off"), ESearchCase::IgnoreCase) || Scalar == TEXT("0"))
	{
		OutValue = false;
		return true;
	}
	return false;
}

bool FKDFNode::TryGetInt(int64& OutValue) const
{
	if (Type != EKDFNodeType::Scalar || Scalar.IsEmpty())
	{
		return false;
	}
	const TCHAR* Start = *Scalar;
	TCHAR* End = nullptr;
	const int64 Value = FCString::Strtoi64(Start, &End, 10);
	if (End == nullptr || *End != TEXT('\0') || End == Start)
	{
		return false;
	}
	OutValue = Value;
	return true;
}

bool FKDFNode::TryGetFloat(double& OutValue) const
{
	if (Type != EKDFNodeType::Scalar || Scalar.IsEmpty())
	{
		return false;
	}
	if (!Scalar.IsNumeric())
	{
		return false;
	}
	OutValue = FCString::Atod(*Scalar);
	return true;
}

FString FKDFNode::GetString(const FString& Default) const { return Type == EKDFNodeType::Scalar ? Scalar : Default; }

FString FKDFNode::GetString(const FString& Key, const FString& Default) const
{
	const FKDFNode* Child = Find(Key);
	return Child ? Child->GetString(Default) : Default;
}

bool FKDFNode::GetBool(const FString& Key, bool bDefault) const
{
	bool bValue = bDefault;
	if (const FKDFNode* Child = Find(Key))
	{
		Child->TryGetBool(bValue);
	}
	return bValue;
}

int64 FKDFNode::GetInt(const FString& Key, int64 Default) const
{
	int64 Value = Default;
	if (const FKDFNode* Child = Find(Key))
	{
		Child->TryGetInt(Value);
	}
	return Value;
}

double FKDFNode::GetFloat(const FString& Key, double Default) const
{
	double Value = Default;
	if (const FKDFNode* Child = Find(Key))
	{
		Child->TryGetFloat(Value);
	}
	return Value;
}

TSharedRef<FKDFNode> FKDFNode::MakeNull() { return MakeShared<FKDFNode>(); }

TSharedRef<FKDFNode> FKDFNode::MakeScalar(const FString& Value, bool bInQuoted)
{
	TSharedRef<FKDFNode> Node = MakeShared<FKDFNode>();
	Node->Type = EKDFNodeType::Scalar;
	Node->Scalar = Value;
	Node->bQuoted = bInQuoted;
	return Node;
}

TSharedRef<FKDFNode> FKDFNode::MakeSequence()
{
	TSharedRef<FKDFNode> Node = MakeShared<FKDFNode>();
	Node->Type = EKDFNodeType::Sequence;
	return Node;
}

TSharedRef<FKDFNode> FKDFNode::MakeMap()
{
	TSharedRef<FKDFNode> Node = MakeShared<FKDFNode>();
	Node->Type = EKDFNodeType::Map;
	return Node;
}

void FKDFNode::SetChild(const FString& Key, const TSharedRef<FKDFNode>& Child)
{
	check(Type == EKDFNodeType::Map);
	for (TTuple<FString, TSharedRef<FKDFNode>>& Pair : Map)
	{
		if (Pair.Key == Key)
		{
			Pair.Value = Child;
			return;
		}
	}
	Map.Emplace(Key, Child);
}

void FKDFNode::AddChild(const TSharedRef<FKDFNode>& Child)
{
	check(Type == EKDFNodeType::Sequence);
	Sequence.Add(Child);
}
