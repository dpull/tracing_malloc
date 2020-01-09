// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

class FExamplePluginEditorModule : public IModuleInterface
{
public:
	void StartupModule() override;

	void ShutdownModule() override;	
};