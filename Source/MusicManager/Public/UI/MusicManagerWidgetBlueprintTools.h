#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MusicManagerWidgetBlueprintTools.generated.h"

/**
 * Editor-only helpers for creating production UMG blueprint hierarchies from automation.
 */
UCLASS()
class MUSICMANAGER_API UMusicManagerWidgetBlueprintTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, CallInEditor, Category="MusicManager|Editor|Widgets")
    static bool RebuildTopStatusBarBlueprint();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="MusicManager|Editor|Widgets")
    static bool RebuildNewsFeedBlueprints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="MusicManager|Editor|Widgets")
    static bool RebuildArtistAuditionPanelBlueprint();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="MusicManager|Editor|Widgets")
    static bool RebuildBottomCommandDockBlueprints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="MusicManager|Editor|Widgets")
    static bool RebuildStudioRecordingBlueprints();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="MusicManager|Editor|Widgets")
    static bool RebuildActiveContractsBlueprints();
};