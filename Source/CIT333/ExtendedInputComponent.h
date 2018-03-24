#pragma once

#include "ExtendedInputComponent.generated.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UENUM(BlueprintType)
enum class EExtendedInputEvent : uint8
{
    Short,
    Long
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_DELEGATE(FExtendedInputActionStaticDelegate);
DECLARE_DYNAMIC_DELEGATE(FExtendedInputActionDynamicDelegate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FExtendedInputAxisDynamicDelegate, float, Value);

class FExtendedInputActionUnifiedDelegate
{
public:
    FExtendedInputActionUnifiedDelegate();
    FExtendedInputActionUnifiedDelegate(FExtendedInputActionStaticDelegate Delegate);
    FExtendedInputActionUnifiedDelegate(FExtendedInputActionDynamicDelegate Delegate);

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

    void BindDelegate(FExtendedInputActionStaticDelegate Delegate);
    void BindDelegate(FExtendedInputActionDynamicDelegate Delegate);

    bool IsBound();

private:
    FExtendedInputActionStaticDelegate StaticDelegate;
    FExtendedInputActionDynamicDelegate DynamicDelegate;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FExtendedInputNameNode;

struct FExtendedInputNameNode
{
    TMap<FName, TUniquePtr<FExtendedInputNameNode>> Children;

    FExtendedInputActionUnifiedDelegate ShortDelegate;
    FExtendedInputActionUnifiedDelegate LongDelegate;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCLASS()
class CIT333_API UExtendedInputComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UExtendedInputComponent();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float ShortTime = 0.05f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float LongTime = 0.5f;

    UFUNCTION(BlueprintCallable, Meta = (DisplayName = "Bind Delayed Action with One Name"))
    void BindDelayedAction1(FName FirstName, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate);

    UFUNCTION(BlueprintCallable, Meta = (DisplayName = "Bind Delayed Action with Two Names"))
    void BindDelayedAction2(FName FirstName, FName SecondName, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate);

    UFUNCTION(BlueprintCallable, Meta = (DisplayName = "Bind Instant Action with One Name"))
    void BindInstantAction(FName Name, FExtendedInputActionDynamicDelegate Delegate);

    UFUNCTION(BlueprintCallable, Meta = (DisplayName = "Bind Instant Axis with One Name"))
    void BindInstantAxis(FName Name, FExtendedInputAxisDynamicDelegate Delegate);

    template<typename UserClass>
    void BindDelayedAction(FName FirstName, EExtendedInputEvent Event, UserClass* Object, typename FExtendedInputActionStaticDelegate::TUObjectMethodDelegate<UserClass>::FMethodPtr Function)
    {
        FExtendedInputActionStaticDelegate Delegate;
        Delegate.BindUObject(Object, Function);

        BindDelayedAction({ FirstName }, Delegate);
    }

    template<typename UserClass>
    void BindDelayedAction(FName FirstName, FName SecondName, EExtendedInputEvent Event, UserClass* Object, typename FExtendedInputActionStaticDelegate::TUObjectMethodDelegate<UserClass>::FMethodPtr Function)
    {
        FExtendedInputActionStaticDelegate Delegate;
        Delegate.BindUObject(Object, Function);

        BindDelayedAction({ FirstName, SecondName }, Delegate);
    }

    template<typename UserClass>
    void BindInstantAction(FName Name, UserClass* Object, typename FExtendedInputActionStaticDelegate::TUObjectMethodDelegate<UserClass>::FMethodPtr Function)
    {
        InputComponent->BindAction(Name, Object, Function);
    }

    template<typename UserClass>
    void BindInstantAxis(FName Name, UserClass* Object, typename FInputAxisHandlerSignature::TUObjectMethodDelegate<UserClass>::FMethodPtr Function)
    {
        InputComponent->BindAxis(Name, Object, Function)
    }

    void SetInputComponent(UInputComponent* InputComponent);

private:
    FExtendedInputNameNode Root;
    FExtendedInputNameNode* Node;
    FTimerHandle ShortHandle;
    FTimerHandle LongHandle;
    FTimerManager* TimerManager;

    TArray<FName> BoundNames;
    TArray<FName> PressedNames;
    TMap<FName, FExtendedInputAxisDynamicDelegate> AxisDelegates;

    UInputComponent* InputComponent;

    void BindDelayedAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionStaticDelegate Delegate);
    void BindDelayedAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate);
    void BindDelayedAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionUnifiedDelegate Delegate);

    void OnNamePressed(FName Name);
    void OnNameReleased(FName Name);
    void OnInstant(FExtendedInputActionDynamicDelegate Delegate);
    void OnShort();
    void OnLong();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};