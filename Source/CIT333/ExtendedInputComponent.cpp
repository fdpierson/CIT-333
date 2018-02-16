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

		// Test if handling bound names this way is a problem or not.
		if (!BoundNames.Contains(Name))
		{
			FInputActionHandlerSignature Signature;
			Signature.BindUObject(this, &UExtendedInputComponent::OnPressed, Name);

			FInputActionBinding Binding;
			Binding.ActionDelegate = Signature;
			Binding.ActionName = Name;
			Binding.KeyEvent = IE_Pressed;

			InputComponent->AddActionBinding(Binding);
			InputComponent->BindAction(Name, IE_Released, this, &UExtendedInputComponent::OnReleased);

			BoundNames.Add(Name);
		}
	}

	if (Event == EExtendedInputEvent::EIE_ShortTap)
	{
		Node->ShortTapDelegate = Delegate;
	}
	else if (Event == EExtendedInputEvent::EIE_LongTap)
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

		// Only remove name?
		PressedNames.Empty();
	}
	else if (TimerManager->IsTimerActive(LongTapHandle))
	{
		TimerManager->ClearTimer(LongTapHandle);
		
		if (Node)
		{
			Node->ShortTapDelegate.Execute();
		}

		// Only remove name?
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
			// Modify?
			PressedNames.Empty();
			return;
		}

		Node = Node->Children.Find(Name)->Get();
	}

	if (Node->LongTapDelegate.IsBound())
	{
		TimerManager->SetTimer(LongTapHandle, this, &UExtendedInputComponent::OnLongTap, LongTapTime, false, LongTapTime);
	}
	else
	{
		Node->ShortTapDelegate.Execute();
	}

	// Modify?
	PressedNames.Empty();
}

void UExtendedInputComponent::OnLongTap()
{
	Node->LongTapDelegate.Execute();
}