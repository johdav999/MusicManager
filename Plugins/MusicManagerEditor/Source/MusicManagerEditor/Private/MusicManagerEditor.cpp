// Copyright Epic Games, Inc. All Rights Reserved.

#include "MusicManagerEditor.h"
#include "MusicManagerEditorStyle.h"
#include "MusicManagerEditorCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

static const FName MusicManagerEditorTabName("MusicManagerEditor");

#define LOCTEXT_NAMESPACE "FMusicManagerEditorModule"

void FMusicManagerEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FMusicManagerEditorStyle::Initialize();
	FMusicManagerEditorStyle::ReloadTextures();

	FMusicManagerEditorCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FMusicManagerEditorCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FMusicManagerEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMusicManagerEditorModule::RegisterMenus));
}

void FMusicManagerEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FMusicManagerEditorStyle::Shutdown();

	FMusicManagerEditorCommands::Unregister();
}

void FMusicManagerEditorModule::PluginButtonClicked()
{
	// Put your "OnButtonClicked" stuff here
	FText DialogText = FText::Format(
							LOCTEXT("PluginButtonDialogText", "Add code to {0} in {1} to override this button's actions"),
							FText::FromString(TEXT("FMusicManagerEditorModule::PluginButtonClicked()")),
							FText::FromString(TEXT("MusicManagerEditor.cpp"))
					   );
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
}

void FMusicManagerEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FMusicManagerEditorCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FMusicManagerEditorCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMusicManagerEditorModule, MusicManagerEditor)