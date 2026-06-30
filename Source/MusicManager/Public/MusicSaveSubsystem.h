#pragma once

#include "CoreMinimal.h"
#include "MusicSaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MusicSaveSubsystem.generated.h"

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
    void SaveGameWithThumbnail(const FString& SlotName, const FSoftObjectPath& ThumbnailAsset);

    UFUNCTION(BlueprintCallable, Category="Save")
    void AutoSave();

    UFUNCTION(BlueprintCallable, Category="Save")
    void LoadGame(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category="Save")
    void GetSaveSlots(TArray<FSaveSlotDescriptor>& OutSlots, bool bIncludeAutosaves = true, bool bIncludeBackups = true);

    UFUNCTION(BlueprintCallable, Category="Save")
    bool DeleteSaveSlot(const FString& SlotName);

    void BuildSaveObject(UMusicSaveGame& SaveObject, const FString& SlotName);
    void BuildSaveObject(UMusicSaveGame& SaveObject, const FString& SlotName, const FSoftObjectPath& ThumbnailAsset);
    FMusicSaveValidationResult ValidateSaveObject(const UMusicSaveGame& SaveObject);
    bool ApplyValidatedSave(const UMusicSaveGame& SaveObject);
    void LogValidationResult(const FMusicSaveValidationResult& Result) const;

protected:
    bool WriteToSlot(UMusicSaveGame* SaveObject, const FString& SlotName);
    UMusicSaveGame* ReadFromSlot(const FString& SlotName);

private:
    static const FString SlotRegistryName;
    static const FString AutoSaveSlotName;

    UMusicSaveSlotRegistry* LoadSlotRegistry() const;
    bool WriteSlotRegistry(UMusicSaveSlotRegistry* Registry) const;
    void RegisterSlotDescriptor(const FSaveSlotDescriptor& Descriptor);
    void CreateBackupForExistingSlot(const FString& SlotName);
    FString BuildBackupSlotName(const FString& SlotName) const;
    bool WriteRawSaveObjectToSlot(USaveGame* SaveObject, const FString& SlotName) const;
};
