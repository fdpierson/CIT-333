#include "ExtendedInputComponent.h"

void UExtendedInputComponent::BeginPlay()
{
	Super::BeginPlay();

	TimerManager = &(GetWorld()->GetTimerManager());
}

void UExtendedInputComponent::SetInputComponent(UInputComponent* InputComponent)
{
	this->InputComponent = InputComponent;
}

void UExtendedInputComponent::AddAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputStaticDelegate Delegate)
{
	AddAction(Names, Event, FExtendedInputUnifiedDelegate(Delegate));
}

void UExtendedInputComponent::AddAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputDynamicDelegate Delegate)
{
	AddAction(Names, Event, FExtendedInputUnifiedDelegate(Delegate));
}

void UExtendedInputComponent::AddAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputUnifiedDelegate Delegate)
{
	FExtendedInputTreeNode* Node = &Root;

	Names.Sort();

	for (FName& Name : Names)
	{
		if (Node->Children.Contains(Name))
		{
			Node = Node->Children.Find(Name)->Get();
		}
		else
		{
			Node = Node->Children.Emplace(Name, new FExtendedInputTreeNode()).Get();
		}

		// FIXME: I believe the blueprint and native class use their own ExtendedInputComponents,
		// yet both the blueprint and native class use the same InputComponent. Also, the below
		// hack removes all bindings that use an action, not just the one assigned by the
		// ExtendedInputComponent.
		//
		// Either find a way to get the blueprint and native class to use the same ExtendedInputComponent
		// or find a way to detect bindings from other ExtendedInputComponents.
		
		for (int32 i = 0; i < InputComponent->GetNumActionBindings(); i++)
		{
			FInputActionBinding& Binding = InputComponent->GetActionBinding(i);

			if (Binding.ActionName == Name)
			{
				InputComponent->RemoveActionBinding(i);
			}
		}

		FInputActionHandlerSignature Signature;
		Signature.BindUObject(this, &UExtendedInputComponent::OnPressed, Name);

		FInputActionBinding Binding;
		Binding.ActionDelegate = Signature;
		Binding.ActionName = Name;
		Binding.KeyEvent = IE_Pressed;

		InputComponent->AddActionBinding(Binding);
		InputComponent->BindAction(Name, IE_Released, this, &UExtendedInputComponent::OnReleased);
	}

	if (Event == EIE_ShortTap)
	{
		Node->ShortTapDelegate = Delegate;
	}
	else if (Event == EIE_LongTap)
	{
		Node->LongTapDelegate = Delegate;
	}
}

void UExtendedInputComponent::OnPressed(FName Name)
{
	if (!TimerManager->IsTimerActive(ShortTapHandle) && !TimerManager->IsTimerActive(LongTapHandle))
	{
		TimerManager->SetTimer(ShortTapHandle, this, &UExtendedInputComponent::OnShortTap, ShortTapTime, false, ShortTapTime);
	}

	if (TimerManager->IsTimerActive(ShortTapHandle))
	{
		PressedNames.Add(Name);
	}
	else if (TimerManager->IsTimerActive(LongTapHandle))
	{
		TimerManager->ClearTimer(LongTapHandle);
		PressedNames.Empty();
	}
}

void UExtendedInputComponent::OnReleased()
{
	if (TimerManager->IsTimerActive(ShortTapHandle))
	{
		TimerManager->ClearTimer(ShortTapHandle);
		PressedNames.Empty();
	}
	else if (TimerManager->IsTimerActive(LongTapHandle))
	{
		TimerManager->ClearTimer(LongTapHandle);
		PressedNames.Empty();
	}
}

void UExtendedInputComponent::OnShortTap()
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

	Node->ShortTapDelegate.Execute();

	if (Node->LongTapDelegate.IsBound())
	{
		TimerManager->SetTimer(LongTapHandle, this, &UExtendedInputComponent::OnLongTap, LongTapTime, false, LongTapTime);
	}

	PressedNames.Empty();
}

void UExtendedInputComponent::OnLongTap()
{
	Node->LongTapDelegate.Execute();
}