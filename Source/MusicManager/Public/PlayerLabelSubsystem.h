#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MusicSaveGame.h"
#include "PlayerLabelSubsystem.generated.h"

/**
 * Owns the player's label identity for campaign-wide systems.
 */
UCLASS()
class MUSICMANAGER_API UPlayerLabelSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintPure, Category="Label")
    FString GetPlayerLabelId() const { return LabelState.LabelId; }

    UFUNCTION(BlueprintPure, Category="Label")
    FString GetPlayerLabelName() const { return LabelState.DisplayName; }

    const FPlayerLabelSnapshot& GetLabelState() const { return LabelState; }

    void BuildSaveSnapshot(FPlayerLabelSnapshot& OutSnapshot) const;
    void ApplySaveSnapshot(const FPlayerLabelSnapshot& Snapshot);
    void ValidateSaveSnapshot(const FPlayerLabelSnapshot& Snapshot, FMusicSaveValidationResult& Result) const;

private:
    UPROPERTY()
    FPlayerLabelSnapshot LabelState;
};
