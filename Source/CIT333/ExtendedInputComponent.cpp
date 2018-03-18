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
    for (auto Delegate : AxisDelegates)
    {
        Delegate.Value.Execute(InputComponent->GetAxisValue(Delegate.Key));
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::BindAction1(FName FirstName, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate)
{
	BindAction({ FirstName }, Event, Delegate);
}

void UExtendedInputComponent::BindAction2(FName FirstName, FName SecondName, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate)
{
	BindAction({ FirstName, SecondName }, Event, Delegate);
}

void UExtendedInputComponent::BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionStaticDelegate Delegate)
{
    BindAction(Names, Event, FExtendedInputActionUnifiedDelegate(Delegate));
}

void UExtendedInputComponent::BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate)
{
    BindAction(Names, Event, FExtendedInputActionUnifiedDelegate(Delegate));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionUnifiedDelegate Delegate)
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
            FInputActionHandlerSignature Signature;
            Signature.BindUObject(this, &UExtendedInputComponent::OnNamePressed, Name);

            FInputActionBinding Binding;
            Binding.ActionDelegate = Signature;
            Binding.ActionName = Name;
            Binding.KeyEvent = IE_Pressed;

            InputComponent->AddActionBinding(Binding);
            InputComponent->BindAction(Name, IE_Released, this, &UExtendedInputComponent::OnNameReleased);
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

void UExtendedInputComponent::BindAxis(FName Name, FExtendedInputAxisDynamicDelegate Delegate)
{
    InputComponent->BindAxis(Name);

    AxisDelegates[Name] = Delegate;
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
        PressedNames.Empty();
    }
}

void UExtendedInputComponent::OnNameReleased()
{
    if (TimerManager->IsTimerActive(ShortHandle))
    {
        TimerManager->ClearTimer(ShortHandle);
        PressedNames.Empty();
    }
    else if (TimerManager->IsTimerActive(LongHandle))
    {
        if (Node)
        {
            Node->ShortDelegate.Execute();
        }

        TimerManager->ClearTimer(LongHandle);
        PressedNames.Empty();
    }
}

void UExtendedInputComponent::OnShort()
{
    Node = &Root;

    PressedNames.Sort();

    for (FName& Name : PressedNames)
    {
        if (!Node->Children.Contains(Name))
        {
            PressedNames.Empty();
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

    PressedNames.Empty();
}

void UExtendedInputComponent::OnLong()
{
    Node->LongDelegate.Execute();
}