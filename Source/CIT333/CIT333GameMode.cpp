// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CIT333GameMode.h"
#include "CIT333Character.h"
#include "UObject/ConstructorHelpers.h"

ACIT333GameMode::ACIT333GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/SideScrollerCPP/Blueprints/SideScrollerCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
