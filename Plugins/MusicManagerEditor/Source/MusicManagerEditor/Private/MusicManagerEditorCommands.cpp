// Copyright Epic Games, Inc. All Rights Reserved.

#include "MusicManagerEditorCommands.h"

#define LOCTEXT_NAMESPACE "FMusicManagerEditorModule"

void FMusicManagerEditorCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "MusicManagerEditor", "Execute MusicManagerEditor action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
