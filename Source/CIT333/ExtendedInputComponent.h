#pragma once

#include "ExtendedInputComponent.generated.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UENUM(BlueprintType)
enum class EExtendedInputEvent : uint8
{
	EIE_ShortTap UMETA(DisplayName = "Short Tap"),
	EIE_LongTap  UMETA(DisplayName = "Long Tap")
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename StaticDelegateType, typename DynamicDelegateType>
class FUnifiedDelegate
{
public:
	FUnifiedDelegate() {}
	FUnifiedDelegate(StaticDelegateType Delegate) : StaticDelegate(Delegate), IsStaticDelegate(true) {}
	FUnifiedDelegate(DynamicDelegateType Delegate) : DynamicDelegate(Delegate), IsStaticDelegate(false) {}

	void BindDelegate(StaticDelegateType Delegate)
	{
		StaticDelegate = Delegate;
	}

	void BindDelegate(DynamicDelegateType Delegate)
	{
		DynamicDelegate = Delegate;
	}

	bool IsBound()
	{
		return StaticDelegate.IsBound() || DynamicDelegate.IsBound();
	}

	void Unbind()
	{
		StaticDelegate.Unbind();
		DynamicDelegate.Unbind();
	}

	void Execute()
	{
		if (IsStaticDelegate)
		{
			StaticDelegate.ExecuteIfBound();
		}
		else
		{
			DynamicDelegate.ExecuteIfBound();
		}
	}

private:
	bool IsStaticDelegate;

	StaticDelegateType StaticDelegate;
	DynamicDelegateType DynamicDelegate;
};

DECLARE_DELEGATE(FExtendedInputStaticDelegate);
DECLARE_DYNAMIC_DELEGATE(FExtendedInputDynamicDelegate);

typedef FUnifiedDelegate<FExtendedInputStaticDelegate, FExtendedInputDynamicDelegate> FExtendedInputUnifiedDelegate;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FExtendedInputTreeNode;

struct FExtendedInputTreeNode
{
	TMap<FName, TUniquePtr<FExtendedInputTreeNode>> Children;

	FExtendedInputUnifiedDelegate ShortTapDelegate;
	FExtendedInputUnifiedDelegate LongTapDelegate;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCLASS()
class CIT333_API UExtendedInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (DisplayName = "Short Tap Time"))
	float ShortTapTime = 0.125f;

	UPROPERTY(meta = (DisplayName = "Long Tap Time"))
	float LongTapTime = 0.500f;

	template<typename ObjType, typename FuncType>
	void AddAction(TArray<FName> Names, EExtendedInputEvent Event, ObjType* Object, FuncType Function)
	{
		FExtendedInputStaticDelegate Delegate;
		Delegate.BindUObject(Object, Function);

		AddAction(Names, Event, Delegate);
	}

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Action"))
	void AddAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputDynamicDelegate Delegate);
	void AddAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputStaticDelegate Delegate);
	void AddAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputUnifiedDelegate Delegate);

	virtual void BeginPlay() override;

	void SetInputComponent(UInputComponent* InputComponent);

protected:
	FExtendedInputTreeNode* Node;
	FExtendedInputTreeNode Root;
	FTimerHandle LongTapHandle;
	FTimerHandle ShortTapHandle;
	FTimerManager* TimerManager;

	TArray<FName> BoundNames;
	TArray<FName> PressedNames;

	UInputComponent* InputComponent;

	void OnPressed(FName Name);
	void OnReleased();
	void OnShortTap();
	void OnLongTap();
};