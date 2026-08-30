

#pragma once

#include "CoreMinimal.h"

class AActor;

template <typename T>
struct FSortedActorDistanceArray
{
	FSortedActorDistanceArray() = default;

	T* PopClosest()
	{
		CleanupInvalid();

		if (Objects.Num() == 0)
		{
			return nullptr;
		}

		TWeakObjectPtr<T> ClosestPtr = Objects.Last();
		Objects.RemoveAt(Objects.Num() - 1);

		return ClosestPtr.Get();
	}

	T* PopClosestValid()
	{
		while (Objects.Num() > 0)
		{
			if (T* Actor = PopClosest())
			{
				return Actor;
			}
		}

		return nullptr;
	}

	void AddObject(T* Actor)
	{
		if (!Actor)
		{
			return;
		}

		for (const TWeakObjectPtr<T>& Existing : Objects)
		{
			if (Existing.Get() == Actor)
			{
				return;
			}
		}

		Objects.Add(TWeakObjectPtr<T>(Actor));
	}

	void RemoveObject(T* Actor)
	{
		if (!Actor)
		{
			return;
		}

		Objects.RemoveAll([Actor](const TWeakObjectPtr<T>& Obj) { return !Obj.IsValid() || Obj.Get() == Actor; });
	}

	void SetSortRef(AActor* ReferenceActor) { SortReferenceActor = ReferenceActor; }

	void Sort()
	{

		CleanupInvalid();

		if (!SortReferenceActor.IsValid())
		{
			return;
		}

		const FVector ReferenceLocation = SortReferenceActor->GetActorLocation();

		Objects.Sort(
			[ReferenceLocation](const TWeakObjectPtr<T>& A, const TWeakObjectPtr<T>& B)
			{
				if (!A.IsValid() || !B.IsValid())
				{
					return A.IsValid();
				}

				const float DistA = FVector::DistSquared(A->GetActorLocation(), ReferenceLocation);
				const float DistB = FVector::DistSquared(B->GetActorLocation(), ReferenceLocation);

				return DistA > DistB;
			});
	}

	TArray<TWeakObjectPtr<T>> GetAllObjects(bool bCleanup = false)
	{
		if (bCleanup)
		{
			CleanupInvalid();
		}
		return Objects;
	}

	int32 Num() const { return Objects.Num(); }

	void Empty()
	{
		Objects.Empty();
		SortReferenceActor.Reset();
	}

	bool IsEmpty() const { return Objects.IsEmpty(); }

private:

	void CleanupInvalid()
	{
		Objects.RemoveAll([](const TWeakObjectPtr<T>& Obj) { return !Obj.IsValid(); });
	}

	TArray<TWeakObjectPtr<T>> Objects;

	TWeakObjectPtr<AActor> SortReferenceActor;
};
