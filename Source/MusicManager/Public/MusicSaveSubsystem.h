#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MusicSaveSubsystem.generated.h"

class UMusicSaveGame;

/**
 * Coordinates serialization and restoration of core game state.
 */
UCLASS()
class MUSICMANAGER_API UMusicSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Save")
    void SaveGame(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category="Save")
    void LoadGame(const FString& SlotName);

protected:
    bool WriteToSlot(UMusicSaveGame* SaveObject, const FString& SlotName);
    UMusicSaveGame* ReadFromSlot(const FString& SlotName);
};
