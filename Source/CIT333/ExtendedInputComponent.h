// TODO: Assign constants and references.
// TODO: Does BindAxis allow multiple delegates to be called for each axis?
// TODO: Fix weird name collisions.

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
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float ShortTime = 0.05f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float LongTime = 0.5f;

    UFUNCTION(BlueprintCallable, Meta = (DisplayName = "Bind Action with One Name"))
    void BindAction1(FName FirstName, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate);

    UFUNCTION(BlueprintCallable, Meta = (DisplayName = "Bind Action with Two Names"))
    void BindAction2(FName FirstName, FName SecondName, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate);

    UFUNCTION(BlueprintCallable)
    void BindAxis(FName Name, FExtendedInputAxisDynamicDelegate Delegate);

    template<typename UserClass>
    void BindAction(FName FirstName, EExtendedInputEvent Event, UserClass* Object, typename FExtendedInputActionStaticDelegate::TUObjectMethodDelegate<UserClass>::FMethodPtr Function)
    {
        FExtendedInputActionStaticDelegate Delegate;
        Delegate.BindUObject(Object, Function);

        BindAction({ FirstName }, Delegate);
    }

    template<typename UserClass>
    void BindAction(FName FirstName, FName SecondName, EExtendedInputEvent Event, UserClass* Object, typename FExtendedInputActionStaticDelegate::TUObjectMethodDelegate<UserClass>::FMethodPtr Function)
    {
        FExtendedInputActionStaticDelegate Delegate;
        Delegate.BindUObject(Object, Function);

        BindAction({ FirstName, SecondName }, Delegate);
    }

    template<typename UserClass>
    void BindAxis(FName Name, UserClass* Object, typename FInputAxisHandlerSignature::TUObjectMethodDelegate<UserClass>::FMethodPtr Function)
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

    void BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionStaticDelegate Delegate);
    void BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate);
    void BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionUnifiedDelegate Delegate);

    void OnNamePressed(FName Name);
    void OnNameReleased();
    void OnShort();
    void OnLong();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};