// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "MusicManagerEditorStyle.h"

class FMusicManagerEditorCommands : public TCommands<FMusicManagerEditorCommands>
{
public:

	FMusicManagerEditorCommands()
		: TCommands<FMusicManagerEditorCommands>(TEXT("MusicManagerEditor"), NSLOCTEXT("Contexts", "MusicManagerEditor", "MusicManagerEditor Plugin"), NAME_None, FMusicManagerEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
