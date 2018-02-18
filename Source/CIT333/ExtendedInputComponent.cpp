#include "ExtendedInputComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::BeginPlay()
{
	Super::BeginPlay();

	TimerManager = &(GetWorld()->GetTimerManager());
}

void UExtendedInputComponent::SetInputComponent(UInputComponent* InputComponent)
{
	this->InputComponent = InputComponent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionStaticDelegate Delegate)
{
	BindAction(Names, Event, FExtendedInputActionUnifiedDelegate(Delegate));
}

void UExtendedInputComponent::BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionDynamicDelegate Delegate)
{
	BindAction(Names, Event, FExtendedInputActionUnifiedDelegate(Delegate));
}

void UExtendedInputComponent::BindAxis(FName Name, FExtendedInputAxisStaticDelegate Delegate)
{
	BindAxis(Name, FExtendedInputAxisUnifiedDelegate(Delegate));
}

void UExtendedInputComponent::BindAxis(FName Name, FExtendedInputAxisDynamicDelegate Delegate)
{
	BindAxis(Name, FExtendedInputAxisUnifiedDelegate(Delegate));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::BindAction(TArray<FName> Names, EExtendedInputEvent Event, FExtendedInputActionUnifiedDelegate Delegate)
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
			Node = Node->Children.Emplace(Name, new FExtendedInputTreeNode).Get();
		}

		if (!BoundActionNames.Contains(Name))
		{
			FInputActionHandlerSignature Signature;
			Signature.BindUObject(this, &UExtendedInputComponent::OnActionPressed, Name);

			FInputActionBinding Binding;
			Binding.ActionDelegate = Signature;
			Binding.ActionName = Name;
			Binding.KeyEvent = IE_Pressed;

			InputComponent->AddActionBinding(Binding);
			InputComponent->BindAction(Name, IE_Released, this, &UExtendedInputComponent::OnActionReleased);
			BoundActionNames.Add(Name);
		}
	}

	if (Event == EExtendedInputEvent::EIE_ShortTap)
	{
		Node->ShortTapDelegate = Delegate;
	}
	else
	{
		Node->LongTapDelegate = Delegate;
	}
}

void UExtendedInputComponent::BindAxis(FName Name, FExtendedInputAxisUnifiedDelegate Delegate)
{
	FExtendedInputTreeNode* Node = nullptr;

	if (Root.Children.Contains(Name))
	{
		Node = Root.Children.Find(Name)->Get();
	}
	else
	{
		Node = Root.Children.Emplace(Name, new FExtendedInputTreeNode).Get();
	}

	if (!BoundAxisNames.Contains(Name))
	{
		FExtendedInputAxisDelegateWrapperDelegate WrapperDelegate;
		WrapperDelegate.BindUObject(this, &UExtendedInputComponent::OnAxisPressed);

		UExtendedInputAxisDelegateWrapper* Wrapper = NewObject<UExtendedInputAxisDelegateWrapper>();
		Wrapper->BindDelegate(WrapperDelegate, Name);

		FInputAxisUnifiedDelegate UnifiedDelegate;
		UnifiedDelegate.BindDelegate(Wrapper, &UExtendedInputAxisDelegateWrapper::Execute);

		FInputAxisBinding Binding;
		Binding.AxisDelegate = UnifiedDelegate;
		Binding.AxisName = Name;

		InputComponent->AxisBindings.Add(Binding);
		BoundAxisNames.Add(Name);
	}

	Node->AxisDelegate = Delegate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::OnActionPressed(FName Name)
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

void UExtendedInputComponent::OnActionReleased()
{
	if (TimerManager->IsTimerActive(ShortTapHandle))
	{
		TimerManager->ClearTimer(ShortTapHandle);
		PressedNames.Empty();
	}
	else if (TimerManager->IsTimerActive(LongTapHandle))
	{
		TimerManager->ClearTimer(LongTapHandle);
		
		if (Node)
		{
			Node->ShortTapDelegate.Execute();
		}

		PressedNames.Empty();
	}
}

void UExtendedInputComponent::OnAxisPressed(FName Name, float Value)
{
	UE_LOG(LogTemp, Log, TEXT("OnAxisPressed: %s %f"), *Name.ToString(), Value);

	Root.Children[Name]->AxisDelegate.Execute(Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UExtendedInputComponent::OnShortTap()
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

	if (Node->LongTapDelegate.IsBound())
	{
		TimerManager->SetTimer(LongTapHandle, this, &UExtendedInputComponent::OnLongTap, LongTapTime, false, LongTapTime);
	}
	else
	{
		Node->ShortTapDelegate.Execute();
	}

	PressedNames.Empty();
}

void UExtendedInputComponent::OnLongTap()
{
	Node->LongTapDelegate.Execute();
}