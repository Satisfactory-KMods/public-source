#pragma once

#include "Async/TaskGraphInterfaces.h"
#include "CoreMinimal.h"

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
