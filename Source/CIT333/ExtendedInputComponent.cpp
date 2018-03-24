#include "ExtendedInputComponent.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FExtendedInputActionUnifiedDelegate::FExtendedInputActionUnifiedDelegate()
{

}

FExtendedInputActionUnifiedDelegate::FExtendedInputActionUnifiedDelegate(FExtendedInputActionStaticDelegate Delegate)
    : StaticDelegate(Delegate)
{

}

FExtendedInputActionUnifiedDelegate::FExtendedInputActionUnifiedDelegate(FExtendedInputActionDynamicDelegate Delegate)
    : DynamicDelegate(Delegate)
{

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FExtendedInputActionUnifiedDelegate::BindDelegate(FExtendedInputActionStaticDelegate Delegate)
{
    StaticDelegate = Delegate;
    DynamicDelegate.Unbind();
}

void FExtendedInputActionUnifiedDelegate::BindDelegate(FExtendedInputActionDynamicDelegate Delegate)
{
    DynamicDelegate = Delegate;
    StaticDelegate.Unbind();
}

bool FExtendedInputActionUnifiedDelegate::IsBound()
{
    return StaticDelegate.IsBound() || DynamicDelegate.IsBound();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UExtendedInputComponent::UExtendedInputComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::SetInputComponent(UInputComponent* InputComponent)
{
    this->InputComponent = InputComponent;
}

void UExtendedInputComponent::BeginPlay()
{
    Super::BeginPlay();

    TimerManager = &(GetWorld()->GetTimerManager());
}

void UExtendedInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    for (auto Delegate : AxisDelegates)
    {
        Delegate.Value.Execute(InputComponent->GetAxisValue(Delegate.Key));
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::BindDelayedAction1(FName FirstName, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate)
{
    BindDelayedAction({ FirstName }, Event, FExtendedInputActionUnifiedDelegate(Delegate));
}

void UExtendedInputComponent::BindDelayedAction2(FName FirstName, FName SecondName, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate)
{
    BindDelayedAction({ FirstName, SecondName }, Event, FExtendedInputActionUnifiedDelegate(Delegate));
}

void UExtendedInputComponent::BindDelayedAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionStaticDelegate Delegate)
{
    BindDelayedAction(Names, Event, FExtendedInputActionUnifiedDelegate(Delegate));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::BindDelayedAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionUnifiedDelegate Delegate)
{
    FExtendedInputNameNode* Node = &Root;

    Names.Sort();

    for (FName& Name : Names)
    {
        if (Node->Children.Contains(Name))
        {
            Node = Node->Children.Find(Name)->Get();
        }
        else
        {
            Node = Node->Children.Emplace(Name, new FExtendedInputNameNode()).Get();
        }

        if (!BoundNames.Contains(Name))
        {
            FInputActionBinding Binding;
            FInputActionHandlerSignature Signature;

            Signature.BindUObject(this, &UExtendedInputComponent::OnNamePressed, Name);

            Binding.ActionDelegate = Signature;
            Binding.ActionName = Name;
            Binding.KeyEvent = IE_Pressed;

            InputComponent->AddActionBinding(Binding);

            Signature.BindUObject(this, &UExtendedInputComponent::OnNameReleased, Name);

            Binding.ActionDelegate = Signature;
            Binding.ActionName = Name;
            Binding.KeyEvent = IE_Released;

            InputComponent->AddActionBinding(Binding);

            BoundNames.Add(Name);
        }
    }

    if (Event == EExtendedInputEvent::Short)
    {
        Node->ShortDelegate = Delegate;
    }
    else
    {
        Node->LongDelegate = Delegate;
    }
}

void UExtendedInputComponent::BindInstantAction(FName Name, FExtendedInputActionDynamicDelegate Delegate)
{
    FInputActionHandlerSignature Signature;
    Signature.BindUObject(this, &UExtendedInputComponent::OnInstant, Delegate);

    FInputActionBinding Binding;
    Binding.ActionDelegate = Signature;
    Binding.ActionName = Name;
    Binding.KeyEvent = IE_Pressed;

    InputComponent->AddActionBinding(Binding);
}

void UExtendedInputComponent::BindInstantAxis(FName Name, FExtendedInputAxisDynamicDelegate Delegate)
{
    InputComponent->BindAxis(Name);

    AxisDelegates.Add(Name, Delegate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::OnNamePressed(FName Name)
{
    if (!TimerManager->IsTimerActive(ShortHandle) && !TimerManager->IsTimerActive(LongHandle))
    {
        TimerManager->SetTimer(ShortHandle, this, &UExtendedInputComponent::OnShort, ShortTime, false, ShortTime);
    }

    if (TimerManager->IsTimerActive(ShortHandle))
    {
        PressedNames.Add(Name);
    }
    else if (TimerManager->IsTimerActive(LongHandle))
    {
        TimerManager->ClearTimer(LongHandle);
    }
}

void UExtendedInputComponent::OnNameReleased(FName Name)
{
    PressedNames.Remove(Name);

    if (TimerManager->IsTimerActive(ShortHandle))
    {
        TimerManager->ClearTimer(ShortHandle);
    }
    else if (TimerManager->IsTimerActive(LongHandle))
    {
        if (Node)
        {
            Node->ShortDelegate.Execute();
        }

        TimerManager->ClearTimer(LongHandle);
    }
}

void UExtendedInputComponent::OnInstant(FExtendedInputActionDynamicDelegate Delegate)
{
    Delegate.Execute();
}

void UExtendedInputComponent::OnShort()
{
    Node = &Root;

    PressedNames.Sort();

    for (FName& Name : PressedNames)
    {
        if (!Node->Children.Contains(Name))
        {
            return;
        }

        Node = Node->Children.Find(Name)->Get();
    }

    if (Node->LongDelegate.IsBound())
    {
        TimerManager->SetTimer(LongHandle, this, &UExtendedInputComponent::OnLong, LongTime, false, LongTime);
    }
    else
    {
        Node->ShortDelegate.Execute();
    }
}

void UExtendedInputComponent::OnLong()
{
    Node->LongDelegate.Execute();
}