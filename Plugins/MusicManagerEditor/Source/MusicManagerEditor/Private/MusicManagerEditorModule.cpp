// Copyright Epic Games, Inc. All Rights Reserved.

#include "MusicManagerEditor.h"
#include "MusicManagerEditorStyle.h"
#include "MusicManagerEditorCommands.h"
#include "RegionMapGeneratorTool.h"
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

	// Create a transient instance of the tool
	URegionMapGeneratorTool* Tool = NewObject<URegionMapGeneratorTool>();

	if (!Tool)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to create RegionMapGeneratorTool instance."));
		return;
	}

	// Run the generator
	Tool->GenerateRegionButtons();

	// Notify editor
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Region Map Buttons generated successfully."));
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
