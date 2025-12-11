// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MusicManagerEditor : ModuleRules
{
    public MusicManagerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[]
            {
                // ... add public include paths required here ...
            }
        );

        PrivateIncludePaths.AddRange(
            new string[]
            {
                // ... add other private include paths required here ...
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",
                "MusicManager"   // Allow editor code to see your runtime classes
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "InputCore",
                "EditorFramework",
                "ToolMenus",
                "UnrealEd",
                "UMGEditor",
                "Slate",
                "SlateCore",
                "Blutility",
                "EditorSubsystem",
                "Kismet",
                "KismetCompiler",
                "KismetWidgets",
                "AssetRegistry",
                "MusicManager"
                // ... add private dependencies that you statically link with here ...
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );
    }
}
