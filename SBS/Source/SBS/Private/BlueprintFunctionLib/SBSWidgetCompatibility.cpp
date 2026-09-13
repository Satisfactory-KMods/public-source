// Copyright Kyri123 / KMods 2026. All Rights Reserved.

#include "BlueprintFunctionLib/SBSWidgetCompatibility.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Patching/NativeHookManager.h"

namespace
{
	struct FWidgetBinding
	{
		TWeakObjectPtr<UClass> mClass;
		FOnWidgetCreated mCallback;
		bool bPreConstruct = false;
	};

	TArray<FWidgetBinding> Bindings;
	FDelegateHandle PreConstructHandle;
	FDelegateHandle ConstructHandle;

	void Dispatch(UUserWidget* Widget, bool bPreConstruct)
	{
		if (!IsValid(Widget) || Widget->IsDesignTime())
		{
			return;
		}
		Bindings.RemoveAll([](const FWidgetBinding& Binding)
						   { return !Binding.mClass.IsValid() || !Binding.mCallback.IsBound(); });

		TArray<FOnWidgetCreated, TInlineAllocator<4>> Callbacks;
		for (const FWidgetBinding& Binding : Bindings)
		{
			const UObject* Target = Binding.mCallback.GetUObject();
			const AActor* Actor = Cast<AActor>(Target);
			if (Binding.bPreConstruct == bPreConstruct && Widget->IsA(Binding.mClass.Get()) && IsValid(Target) &&
				(!Actor || !Actor->IsActorBeingDestroyed()) && Target->GetWorld() == Widget->GetWorld())
			{
				Callbacks.Add(Binding.mCallback);
			}
		}
		for (FOnWidgetCreated& Callback : Callbacks)
		{
			if (IsValid(Widget))
			{
				Callback.ExecuteIfBound(Widget);
			}
		}
	}

	class FWidgetHookAccess : public UUserWidget
	{
	public:
		static void Install()
		{
#if !UE_SERVER
			if (!PreConstructHandle.IsValid())
			{
				PreConstructHandle =
					SUBSCRIBE_METHOD_VIRTUAL_AFTER(UUserWidget::NativePreConstruct, GetMutableDefault<UUserWidget>(),
												   [](UUserWidget* Widget) { Dispatch(Widget, true); });
				ConstructHandle =
					SUBSCRIBE_METHOD_VIRTUAL_AFTER(UUserWidget::NativeConstruct, GetMutableDefault<UUserWidget>(),
												   [](UUserWidget* Widget) { Dispatch(Widget, false); });
			}
#endif
		}

		static void Remove()
		{
#if !UE_SERVER
			if (PreConstructHandle.IsValid())
			{
				UNSUBSCRIBE_METHOD(UUserWidget::NativePreConstruct, PreConstructHandle);
				UNSUBSCRIBE_METHOD(UUserWidget::NativeConstruct, ConstructHandle);
				PreConstructHandle.Reset();
				ConstructHandle.Reset();
			}
#endif
		}
	};

	void Bind(const TSubclassOf<UUserWidget> WidgetClass, FOnWidgetCreated Callback, bool bPreConstruct)
	{
		UObject* Target = Callback.GetUObject();
		if (!WidgetClass || !IsValid(Target) || !Callback.IsBound() || IsRunningCommandlet() ||
			IsRunningDedicatedServer() || !Target->GetWorld() || !Target->GetWorld()->IsGameWorld())
		{
			return;
		}
		Bindings.RemoveAll([](const FWidgetBinding& Entry)
						   { return !Entry.mCallback.IsBound() || !Entry.mClass.IsValid(); });
		if (Bindings.ContainsByPredicate(
				[&](const FWidgetBinding& Entry)
				{
					return Entry.mClass == WidgetClass.Get() && Entry.bPreConstruct == bPreConstruct &&
						Entry.mCallback == Callback;
				}))
		{
			return;
		}
		FWidgetHookAccess::Install();
		Bindings.Add({WidgetClass.Get(), MoveTemp(Callback), bPreConstruct});
	}
}

void USBSWidgetCompatibility::BindOnWidget(const TSubclassOf<UUserWidget> WidgetClass, FOnWidgetCreated Binding)
{
	Bind(WidgetClass, MoveTemp(Binding), false);
}

void USBSWidgetCompatibility::BindOnPreWidget(const TSubclassOf<UUserWidget> WidgetClass, FOnWidgetCreated Binding)
{
	Bind(WidgetClass, MoveTemp(Binding), true);
}

void USBSWidgetCompatibility::ShutdownHooks()
{
	Bindings.Reset();
	FWidgetHookAccess::Remove();
}
