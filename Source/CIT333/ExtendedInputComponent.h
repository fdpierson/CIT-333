#pragma once

#include "ExtendedInputComponent.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UENUM(BlueprintType)
enum class EExtendedInputEvent : uint8
{
	EIE_ShortTap UMETA(DisplayName = "Short Tap"),
	EIE_LongTap  UMETA(DisplayName = "Long Tap")
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_DELEGATE(FExtendedInputActionStaticDelegate);
DECLARE_DELEGATE_OneParam(FExtendedInputAxisStaticDelegate, float);
DECLARE_DYNAMIC_DELEGATE(FExtendedInputActionDynamicDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FExtendedInputAxisDynamicDelegate, float, Value);

template<typename StaticDelegateType, typename DynamicDelegateType>
class TExtendedInputUnifiedDelegate
{
public:
	TExtendedInputUnifiedDelegate() {}
	TExtendedInputUnifiedDelegate(StaticDelegateType Delegate) : StaticDelegate(Delegate) {}
	TExtendedInputUnifiedDelegate(DynamicDelegateType Delegate) : DynamicDelegate(Delegate) {}

	void BindDelegate(StaticDelegateType Delegate)
	{
		StaticDelegate = Delegate;
		DynamicDelegate.Unbind();
	}

	void BindDelegate(DynamicDelegateType Delegate)
	{
		DynamicDelegate = Delegate;
		StaticDelegate.Unbind();
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

	template<typename... VarTypes>
	void Execute(VarTypes... Variables)
	{
		if (StaticDelegate.IsBound())
		{
			StaticDelegate.Execute(Variables...);
		}
		else if (DynamicDelegate.IsBound())
		{
			DynamicDelegate.Execute(Variables...);
		}
	}

private:
	StaticDelegateType StaticDelegate;
	DynamicDelegateType DynamicDelegate;
};

typedef TExtendedInputUnifiedDelegate<FExtendedInputActionStaticDelegate, FExtendedInputActionDynamicDelegate> FExtendedInputActionUnifiedDelegate;
typedef TExtendedInputUnifiedDelegate<FExtendedInputAxisStaticDelegate, FExtendedInputAxisDynamicDelegate> FExtendedInputAxisUnifiedDelegate;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_DELEGATE_TwoParams(FExtendedInputAxisDelegateWrapperDelegate, FName, float);

UCLASS()
class CIT333_API UExtendedInputAxisDelegateWrapper : public UObject
{
	GENERATED_BODY()

public:
	void BindDelegate(FExtendedInputAxisDelegateWrapperDelegate Delegate, FName Name)
	{
		this->Delegate = Delegate;
		this->Name = Name;
	}

	void Execute(float Value)
	{
		if (Delegate.IsBound())
		{
			Delegate.Execute(Name, Value);
		}
	}

private:
	FExtendedInputAxisDelegateWrapperDelegate Delegate;

	FName Name;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FExtendedInputTreeNode;

struct FExtendedInputTreeNode
{
	TMap<FName, TUniquePtr<FExtendedInputTreeNode>> Children;

	FExtendedInputActionUnifiedDelegate ShortTapDelegate;
	FExtendedInputActionUnifiedDelegate LongTapDelegate;
	FExtendedInputAxisUnifiedDelegate AxisDelegate;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCLASS()
class CIT333_API UExtendedInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (DisplayName = "Short Tap Time"))
	float ShortTapTime = 0.050f;

	UPROPERTY(meta = (DisplayName = "Long Tap Time"))
	float LongTapTime = 0.500f;

	template<typename ObjType, typename FuncType>
	void BindAction(TArray<FName> Names, EExtendedInputEvent Event, ObjType* Object, FuncType Function)
	{
		FExtendedInputActionStaticDelegate Delegate;
		Delegate.BindUObject(Object, Function);

		BindAction(Names, Event, Delegate);
	}

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Bind Action"))
	void BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate);
	void BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionStaticDelegate Delegate);
	void BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionUnifiedDelegate Delegate);

	template<typename ObjType, typename FuncType>
	void BindAxis(FName Name, ObjType* Object, FuncType Function, bool Immediate)
	{
		FExtendedInputAxisStaticDelegate Delegate;
		Delegate.BindUObject(Object, Function);

		BindAxis(Names, Event, Delegate, Immediate);
	}

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Bind Axis"))
	void BindAxis(FName Name, FExtendedInputAxisDynamicDelegate Delegate);
	void BindAxis(FName Name, FExtendedInputAxisStaticDelegate Delegate);
	void BindAxis(FName Name, FExtendedInputAxisUnifiedDelegate Delegate);

	virtual void BeginPlay() override;

	void SetInputComponent(UInputComponent* InputComponent);

protected:
	FExtendedInputTreeNode* Node;
	FExtendedInputTreeNode Root;
	FTimerHandle LongTapHandle;
	FTimerHandle ShortTapHandle;
	FTimerManager* TimerManager;

	TArray<FName> BoundActionNames;
	TArray<FName> BoundAxisNames;
	TArray<FName> PressedNames;

	UInputComponent* InputComponent;

	void OnActionPressed(FName Name);
	void OnActionReleased();
	void OnAxisPressed(FName Name, float Value);
	void OnShortTap();
	void OnLongTap();
};