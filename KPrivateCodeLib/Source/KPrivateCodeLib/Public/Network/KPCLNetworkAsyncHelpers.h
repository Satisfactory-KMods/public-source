// ILikeBanas

#pragma once

#include "Async/TaskGraphInterfaces.h"
#include "CoreMinimal.h"

/**
 * Safely dispatches a callable to the game thread, guarding against use-after-free.
 *
 * Captures Obj as a TWeakObjectPtr so the lambda is a no-op if the object has
 * been garbage-collected or destroyed before the game-thread task runs.
 *
 * Usage:
 *   RunOnGameThreadIfValid(this, [](AMyActor* Self) { Self->DoThing(); });
 *
 * The FnType template parameter is deduced directly from the callable so raw
 * lambdas can be passed without wrapping them in TFunction first.
 */
template <typename T, typename FnType>
void RunOnGameThreadIfValid(T* Obj, FnType&& Fn)
{
	TWeakObjectPtr<T> WeakObj(Obj);
	AsyncTask(ENamedThreads::GameThread,
			  [WeakObj, Fn = Forward<FnType>(Fn)]()
			  {
				  if (T* StrongObj = WeakObj.Get())
				  {
					  Fn(StrongObj);
				  }
			  });
}

/**
 * Safely dispatches a callable to the game thread for a non-UObject raw pointer
 * guarded by a TWeakObjectPtr sentinel. Use when the lambda only needs a plain
 * pointer (Ptr) but liveness is determined by a UObject (Sentinel).
 *
 * The lambda is a no-op if Sentinel is no longer alive.
 */
template <typename TSentinel, typename TPtr, typename FnType>
void RunOnGameThreadIfValidRaw(TSentinel* Sentinel, TPtr* Ptr, FnType&& Fn)
{
	TWeakObjectPtr<TSentinel> WeakSentinel(Sentinel);
	AsyncTask(ENamedThreads::GameThread,
			  [WeakSentinel, Ptr, Fn = Forward<FnType>(Fn)]()
			  {
				  if (WeakSentinel.IsValid())
				  {
					  Fn(Ptr);
				  }
			  });
}
