// Copyright Epic Games, Inc. All Rights Reserved.

#include "MusicManagerEditor.h"
#include "MusicManagerEditorStyle.h"
#include "MusicManagerEditorCommands.h"

#define LOCTEXT_NAMESPACE "FMusicManagerEditorModule"

void FMusicManagerEditorModule::StartupModule()
{
        FMusicManagerEditorStyle::Initialize();
        FMusicManagerEditorStyle::ReloadTextures();

        FMusicManagerEditorCommands::Register();
}

void FMusicManagerEditorModule::ShutdownModule()
{
        // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
        // we call this function before unloading the module.

        FMusicManagerEditorStyle::Shutdown();

        FMusicManagerEditorCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMusicManagerEditorModule, MusicManagerEditor)
