// Copyright (C) Simone Di Gravio <email: altairjp@gmail.com> - All Rights Reserved

#pragma once

#include "Modules/ModuleManager.h"
#include "Engine.h"

class FTwitchPlayModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};