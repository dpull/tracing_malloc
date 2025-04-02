// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ExamplePlugin.h"
#include "ExampleComponent.h"

void AddGameModeComponent(AActor* Actor)
{
    Actor->AddOwnedComponent(Actor->CreateDefaultSubobject<UExampleComponent>(TEXT("ExampleComponent")));
}

void AddCharacterComponent(AActor* Actor)
{
}
