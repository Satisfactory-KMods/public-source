// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * A template container that stores actors and sorts them by distance to a reference actor.
 * Useful for proximity-based loading systems.
 * @tparam T The actor type to store (must derive from AActor)
 */
template <typename T>
struct FSortedActorDistanceArray
{
	FSortedActorDistanceArray() = default;

	/**
	 * Removes and returns the closest actor to the reference point.
	 * The array must be sorted before calling this method.
	 * @return The closest actor, or nullptr if the array is empty
	 */
	T* PopClosest()
	{
		CleanupInvalid();

		if (Objects.Num() == 0)
		{
			return nullptr;
		}

		// Get last element (closest to reference due to ascending sort)
		TWeakObjectPtr<T> ClosestPtr = Objects.Last();
		Objects.RemoveAt(Objects.Num() - 1);

		return ClosestPtr.Get();
	}

	/**
	 * Removes and returns the closest VALID actor to the reference point.
	 * Automatically skips over invalid pointers until a valid actor is found.
	 * The array must be sorted before calling this method.
	 * @return The closest valid actor, or nullptr if no valid actors remain
	 */
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

	/**
	 * Adds an actor to the container.
	 * @param Actor The actor to add
	 */
	void AddObject(T* Actor)
	{
		if (!Actor)
		{
			return;
		}

		// Avoid duplicates
		for (const TWeakObjectPtr<T>& Existing : Objects)
		{
			if (Existing.Get() == Actor)
			{
				return;
			}
		}

		Objects.Add(TWeakObjectPtr<T>(Actor));
	}

	/**
	 * Removes an actor from the container.
	 * @param Actor The actor to remove
	 */
	void RemoveObject(T* Actor)
	{
		if (!Actor)
		{
			return;
		}

		Objects.RemoveAll([Actor](const TWeakObjectPtr<T>& Obj) { return !Obj.IsValid() || Obj.Get() == Actor; });
	}

	/**
	 * Sets the reference actor used for distance calculations.
	 * @param ReferenceActor The actor to use as reference point
	 */
	void SetSortRef(AActor* ReferenceActor) { SortReferenceActor = ReferenceActor; }

	/**
	 * Sorts all objects by distance to the reference actor.
	 * Closest objects will be at the end of the array.
	 */
	void Sort()
	{
		// Clean up invalid references first
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

				// Sort ascending (furthest first), so closest are at the end
				return DistA > DistB;
			});
	}

	/**
	 * Returns all stored objects.
	 * @param bCleanup If true, removes invalid weak pointers before returning
	 * @return Array of weak object pointers
	 */
	TArray<TWeakObjectPtr<T>> GetAllObjects(bool bCleanup = false)
	{
		if (bCleanup)
		{
			CleanupInvalid();
		}
		return Objects;
	}

	/**
	 * Returns the number of objects in the container.
	 * @return Number of objects
	 */
	int32 Num() const { return Objects.Num(); }

	/**
	 * Removes all objects from the container.
	 */
	void Empty()
	{
		Objects.Empty();
		SortReferenceActor.Reset();
	}

	/**
	 * Checks if the container is empty.
	 * @return True if empty
	 */
	bool IsEmpty() const { return Objects.IsEmpty(); }

private:
	/** Removes all invalid weak pointers from the array. */
	void CleanupInvalid()
	{
		Objects.RemoveAll([](const TWeakObjectPtr<T>& Obj) { return !Obj.IsValid(); });
	}

	/** The stored actors. */
	TArray<TWeakObjectPtr<T>> Objects;

	/** The reference actor for distance calculations. */
	TWeakObjectPtr<AActor> SortReferenceActor;
};
