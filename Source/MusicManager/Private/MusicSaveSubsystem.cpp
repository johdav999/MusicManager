#include "MusicSaveSubsystem.h"

#include "ArtistManagerSubsystem.h"
#include "Async/Async.h"
#include "ChartManagerSubsystem.h"
#include "GameTimeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "MarketingManagerSubsystem.h"
#include "MarketManagerSubsystem.h"
#include "MusicSaveGame.h"
#include "PlayerLabelSubsystem.h"
#include "RecordManagerSubsystem.h"
#include "FinanceManagerSubsystem.h"
#include "EventSubsystem.h"
#include "SongManagerSubsystem.h"
#include "UIManagerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMusicSaveSubsystem, Log, All);

const FString UMusicSaveSubsystem::SlotRegistryName = TEXT("MusicManager_SaveSlotRegistry");
const FString UMusicSaveSubsystem::AutoSaveSlotName = TEXT("MusicManager_Autosave");

void UMusicSaveSubsystem::SaveGame(const FString& SlotName)
{
    SaveGameWithThumbnail(SlotName, FSoftObjectPath());
}

void UMusicSaveSubsystem::SaveGameWithThumbnail(const FString& SlotName, const FSoftObjectPath& ThumbnailAsset)
{
    if (!IsInGameThread())
    {
        const FString SafeSlot = SlotName;
        const FSoftObjectPath SafeThumbnailAsset = ThumbnailAsset;
        TWeakObjectPtr<UMusicSaveSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, SafeSlot, SafeThumbnailAsset]()
        {
            if (UMusicSaveSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->SaveGameWithThumbnail(SafeSlot, SafeThumbnailAsset);
            }
        });
        return;
    }

    if (SlotName.IsEmpty())
    {
        UE_LOG(LogMusicSaveSubsystem, Warning, TEXT("SaveGame called with empty slot name."));
        return;
    }

    CreateBackupForExistingSlot(SlotName);

    UMusicSaveGame* SaveObject = NewObject<UMusicSaveGame>();
    if (!SaveObject)
    {
        UE_LOG(LogMusicSaveSubsystem, Error, TEXT("Failed to allocate save game object."));
        return;
    }

    BuildSaveObject(*SaveObject, SlotName, ThumbnailAsset);

    if (WriteToSlot(SaveObject, SlotName))
    {
        RegisterSlotDescriptor(SaveObject->BuildSlotDescriptor(SlotName, false, false));
        UE_LOG(LogMusicSaveSubsystem, Log, TEXT("Saved game to slot '%s'."), *SlotName);
    }
}

void UMusicSaveSubsystem::AutoSave()
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UMusicSaveSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (UMusicSaveSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->AutoSave();
            }
        });
        return;
    }

    UMusicSaveGame* SaveObject = NewObject<UMusicSaveGame>();
    if (!SaveObject)
    {
        UE_LOG(LogMusicSaveSubsystem, Error, TEXT("Failed to allocate autosave object."));
        return;
    }

    BuildSaveObject(*SaveObject, AutoSaveSlotName, FSoftObjectPath());

    if (WriteToSlot(SaveObject, AutoSaveSlotName))
    {
        RegisterSlotDescriptor(SaveObject->BuildSlotDescriptor(AutoSaveSlotName, true, false));
        UE_LOG(LogMusicSaveSubsystem, Log, TEXT("Autosaved game to slot '%s'."), *AutoSaveSlotName);
    }
}

void UMusicSaveSubsystem::LoadGame(const FString& SlotName)
{
    if (!IsInGameThread())
    {
        const FString SafeSlot = SlotName;
        TWeakObjectPtr<UMusicSaveSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, SafeSlot]()
        {
            if (UMusicSaveSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->LoadGame(SafeSlot);
            }
        });
        return;
    }

    if (SlotName.IsEmpty())
    {
        UE_LOG(LogMusicSaveSubsystem, Warning, TEXT("LoadGame called with empty slot name."));
        return;
    }

    UMusicSaveGame* SaveObject = ReadFromSlot(SlotName);
    if (!SaveObject)
    {
        UE_LOG(LogMusicSaveSubsystem, Warning, TEXT("No save object could be loaded from slot '%s'."), *SlotName);
        return;
    }

    FMusicSaveValidationResult MigrationResult = SaveObject->MigrateToCurrentVersion();
    LogValidationResult(MigrationResult);
    if (!MigrationResult.bIsValid)
    {
        UE_LOG(LogMusicSaveSubsystem, Error, TEXT("LoadGame aborted for slot '%s' because migration failed."), *SlotName);
        return;
    }

    FMusicSaveValidationResult ValidationResult = ValidateSaveObject(*SaveObject);
    LogValidationResult(ValidationResult);
    if (!ValidationResult.bIsValid)
    {
        UE_LOG(LogMusicSaveSubsystem, Error, TEXT("LoadGame aborted for slot '%s' because validation failed."), *SlotName);
        return;
    }

    if (ApplyValidatedSave(*SaveObject))
    {
        UE_LOG(LogMusicSaveSubsystem, Log, TEXT("Loaded game from slot '%s'."), *SlotName);
    }
}

void UMusicSaveSubsystem::GetSaveSlots(TArray<FSaveSlotDescriptor>& OutSlots, bool bIncludeAutosaves, bool bIncludeBackups)
{
    OutSlots.Reset();

    UMusicSaveSlotRegistry* Registry = LoadSlotRegistry();
    if (!Registry)
    {
        return;
    }

    for (FSaveSlotDescriptor Descriptor : Registry->Registry.Slots)
    {
        if ((!bIncludeAutosaves && Descriptor.bIsAutosave) || (!bIncludeBackups && Descriptor.bIsBackup))
        {
            continue;
        }

        if (UMusicSaveGame* SaveObject = ReadFromSlot(Descriptor.SlotName))
        {
            FMusicSaveValidationResult MigrationResult = SaveObject->MigrateToCurrentVersion();
            FMusicSaveValidationResult ValidationResult = MigrationResult.bIsValid ? ValidateSaveObject(*SaveObject) : MigrationResult;
            Descriptor = SaveObject->BuildSlotDescriptor(Descriptor.SlotName, Descriptor.bIsAutosave, Descriptor.bIsBackup, Descriptor.ParentSlotName);
            Descriptor.bIsLoadable = ValidationResult.bIsValid;
            Descriptor.ValidationMessages.Reset();
            Descriptor.ValidationMessages.Append(MigrationResult.Errors);
            Descriptor.ValidationMessages.Append(MigrationResult.Warnings);
            Descriptor.ValidationMessages.Append(ValidationResult.Errors);
            Descriptor.ValidationMessages.Append(ValidationResult.Warnings);
        }
        else
        {
            Descriptor.bIsLoadable = false;
            Descriptor.ValidationMessages = { TEXT("Registered save slot could not be read.") };
        }

        OutSlots.Add(Descriptor);
    }

    OutSlots.Sort([](const FSaveSlotDescriptor& A, const FSaveSlotDescriptor& B)
    {
        return A.LastSavedAt > B.LastSavedAt;
    });
}

bool UMusicSaveSubsystem::DeleteSaveSlot(const FString& SlotName)
{
    if (SlotName.IsEmpty())
    {
        return false;
    }

    const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, 0);
    UMusicSaveSlotRegistry* Registry = LoadSlotRegistry();
    if (Registry)
    {
        Registry->Registry.Slots.RemoveAll([&SlotName](const FSaveSlotDescriptor& Descriptor)
        {
            return Descriptor.SlotName == SlotName;
        });
        WriteSlotRegistry(Registry);
    }

    return bDeleted;
}

void UMusicSaveSubsystem::BuildSaveObject(UMusicSaveGame& SaveObject, const FString& SlotName)
{
    BuildSaveObject(SaveObject, SlotName, FSoftObjectPath());
}

void UMusicSaveSubsystem::BuildSaveObject(UMusicSaveGame& SaveObject, const FString& SlotName, const FSoftObjectPath& ThumbnailAsset)
{
    SaveObject.InitializeNewSave(SlotName);
    SaveObject.CampaignMeta.ThumbnailAsset = ThumbnailAsset;

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogMusicSaveSubsystem, Error, TEXT("BuildSaveObject failed: no game instance."));
        return;
    }

    if (UPlayerLabelSubsystem* LabelSubsystem = GameInstance->GetSubsystem<UPlayerLabelSubsystem>())
    {
        LabelSubsystem->BuildSaveSnapshot(SaveObject.PlayerLabelSnapshot);
    }

    if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
    {
        TimeSubsystem->BuildSaveSnapshot(SaveObject.TimeSnapshot);
        SaveObject.SavedGameDate = SaveObject.TimeSnapshot.CurrentGameDate;
    }

    if (USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>())
    {
        SongManager->SerializeForSave(SaveObject.SavedSongs);
    }

    if (UArtistManagerSubsystem* ArtistManager = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
    {
        ArtistManager->BuildSaveSnapshot(SaveObject.ArtistSnapshot);
        SaveObject.SavedContracts = SaveObject.ArtistSnapshot.ActiveContracts;
    }

    if (URecordManagerSubsystem* RecordManager = GameInstance->GetSubsystem<URecordManagerSubsystem>())
    {
        RecordManager->BuildSaveSnapshot(SaveObject.RecordSnapshot);
    }

    if (UFinanceManagerSubsystem* FinanceManager = GameInstance->GetSubsystem<UFinanceManagerSubsystem>())
    {
        FinanceManager->BuildSaveSnapshot(SaveObject.FinanceSnapshot);
    }

    if (UMarketManagerSubsystem* MarketManager = GameInstance->GetSubsystem<UMarketManagerSubsystem>())
    {
        MarketManager->BuildSaveSnapshot(SaveObject.MarketSnapshot);
    }

    if (UMarketingManagerSubsystem* MarketingManager = GameInstance->GetSubsystem<UMarketingManagerSubsystem>())
    {
        MarketingManager->BuildSaveSnapshot(SaveObject.MarketingSnapshot);
    }

    if (UChartManagerSubsystem* ChartManager = GameInstance->GetSubsystem<UChartManagerSubsystem>())
    {
        ChartManager->BuildSaveSnapshot(SaveObject.ChartSnapshot);
    }

    if (UEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UEventSubsystem>())
    {
        EventSubsystem->BuildSaveSnapshot(SaveObject.NewsSnapshot);
    }
}

FMusicSaveValidationResult UMusicSaveSubsystem::ValidateSaveObject(const UMusicSaveGame& SaveObject)
{
    FMusicSaveValidationResult Result = SaveObject.ValidateTopLevel();

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        Result.AddError(TEXT("Cannot validate save: game instance is unavailable."));
        return Result;
    }

    UPlayerLabelSubsystem* LabelSubsystem = GameInstance->GetSubsystem<UPlayerLabelSubsystem>();
    UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>();
    USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>();
    UArtistManagerSubsystem* ArtistManager = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    URecordManagerSubsystem* RecordManager = GameInstance->GetSubsystem<URecordManagerSubsystem>();
    UFinanceManagerSubsystem* FinanceManager = GameInstance->GetSubsystem<UFinanceManagerSubsystem>();
    UMarketManagerSubsystem* MarketManager = GameInstance->GetSubsystem<UMarketManagerSubsystem>();
    UMarketingManagerSubsystem* MarketingManager = GameInstance->GetSubsystem<UMarketingManagerSubsystem>();
    UChartManagerSubsystem* ChartManager = GameInstance->GetSubsystem<UChartManagerSubsystem>();
    UEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UEventSubsystem>();

    if (!LabelSubsystem) { Result.AddError(TEXT("PlayerLabelSubsystem is unavailable.")); }
    if (!TimeSubsystem) { Result.AddError(TEXT("GameTimeSubsystem is unavailable.")); }
    if (!SongManager) { Result.AddError(TEXT("SongManagerSubsystem is unavailable.")); }
    if (!ArtistManager) { Result.AddError(TEXT("ArtistManagerSubsystem is unavailable.")); }
    if (!RecordManager) { Result.AddError(TEXT("RecordManagerSubsystem is unavailable.")); }
    if (!FinanceManager) { Result.AddError(TEXT("FinanceManagerSubsystem is unavailable.")); }
    if (!MarketManager) { Result.AddError(TEXT("MarketManagerSubsystem is unavailable.")); }
    if (!MarketingManager) { Result.AddError(TEXT("MarketingManagerSubsystem is unavailable.")); }
    if (!ChartManager) { Result.AddError(TEXT("ChartManagerSubsystem is unavailable.")); }
    if (!EventSubsystem) { Result.AddError(TEXT("EventSubsystem is unavailable.")); }

    if (!Result.bIsValid)
    {
        return Result;
    }

    LabelSubsystem->ValidateSaveSnapshot(SaveObject.PlayerLabelSnapshot, Result);
    TimeSubsystem->ValidateSaveSnapshot(SaveObject.TimeSnapshot, Result);
    SongManager->ValidateSaveRecords(SaveObject.SavedSongs, Result);

    TSet<FString> KnownSongIds;
    TSet<FString> KnownArtistIds;
    TSet<FString> KnownRecordIds;
    TSet<FString> KnownLabelIds;
    TSet<FString> KnownRegionIds;

    KnownLabelIds.Add(SaveObject.PlayerLabelSnapshot.LabelId);

    for (const FSongSaveRecord& SongRecord : SaveObject.SavedSongs)
    {
        if (!SongRecord.SongId.IsEmpty())
        {
            KnownSongIds.Add(SongRecord.SongId);
        }
        if (!SongRecord.ArtistId.IsEmpty())
        {
            KnownArtistIds.Add(SongRecord.ArtistId);
        }
        if (!SongRecord.RecordedOnRecordId.IsEmpty())
        {
            KnownRecordIds.Add(SongRecord.RecordedOnRecordId);
        }
    }

    auto AddArtistData = [&KnownArtistIds](const FArtistData& Artist)
    {
        if (!Artist.ArtistId.IsEmpty())
        {
            KnownArtistIds.Add(Artist.ArtistId);
        }
        if (!Artist.ArtistName.IsEmpty())
        {
            KnownArtistIds.Add(Artist.ArtistName);
        }
    };

    for (const FArtistData& Artist : SaveObject.ArtistSnapshot.UnsignedArtists)
    {
        AddArtistData(Artist);
    }
    for (const FArtistContract& Contract : SaveObject.ArtistSnapshot.ActiveContracts)
    {
        if (!Contract.ArtistId.IsEmpty())
        {
            KnownArtistIds.Add(Contract.ArtistId);
        }
        AddArtistData(Contract.ArtistData);
    }
    for (const FArtistContract& Contract : SaveObject.ArtistSnapshot.ExpiredContracts)
    {
        if (!Contract.ArtistId.IsEmpty())
        {
            KnownArtistIds.Add(Contract.ArtistId);
        }
        AddArtistData(Contract.ArtistData);
    }

    for (const FRecordData& Record : SaveObject.RecordSnapshot.Records)
    {
        if (!Record.RecordId.IsEmpty())
        {
            KnownRecordIds.Add(Record.RecordId);
        }
        if (!Record.LabelId.IsEmpty())
        {
            KnownLabelIds.Add(Record.LabelId);
        }
    }

    for (const TPair<FString, FMarketRegion>& RegionPair : MarketManager->LoadedRegions)
    {
        if (!RegionPair.Key.IsEmpty())
        {
            KnownRegionIds.Add(RegionPair.Key);
        }
    }

    ArtistManager->ValidateSaveSnapshot(SaveObject.ArtistSnapshot, KnownSongIds, Result);
    RecordManager->ValidateSaveSnapshot(SaveObject.RecordSnapshot, KnownArtistIds, KnownSongIds, KnownLabelIds, Result);
    FinanceManager->ValidateSaveSnapshot(SaveObject.FinanceSnapshot, KnownLabelIds, Result);
    MarketManager->ValidateSaveSnapshot(SaveObject.MarketSnapshot, KnownArtistIds, KnownRecordIds, Result);
    MarketingManager->ValidateSaveSnapshot(SaveObject.MarketingSnapshot, KnownRecordIds, KnownLabelIds, KnownRegionIds, Result);
    ChartManager->ValidateSaveSnapshot(SaveObject.ChartSnapshot, KnownRecordIds, KnownArtistIds, KnownRegionIds, Result);
    EventSubsystem->ValidateSaveSnapshot(SaveObject.NewsSnapshot, Result);

    const FMusicSaveValidationResult FutureValidation = SaveObject.ValidateFutureSystemsSnapshot();
    Result.Errors.Append(FutureValidation.Errors);
    Result.Warnings.Append(FutureValidation.Warnings);
    if (!FutureValidation.bIsValid)
    {
        Result.bIsValid = false;
    }

    for (const FSongSaveRecord& SongRecord : SaveObject.SavedSongs)
    {
        if (!SongRecord.RecordedOnRecordId.IsEmpty() && !KnownRecordIds.Contains(SongRecord.RecordedOnRecordId))
        {
            Result.AddError(FString::Printf(TEXT("Saved song %s references missing record %s."), *SongRecord.SongId, *SongRecord.RecordedOnRecordId));
        }
    }

    return Result;
}

bool UMusicSaveSubsystem::ApplyValidatedSave(const UMusicSaveGame& SaveObject)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return false;
    }

    UPlayerLabelSubsystem* LabelSubsystem = GameInstance->GetSubsystem<UPlayerLabelSubsystem>();
    UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>();
    USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>();
    UArtistManagerSubsystem* ArtistManager = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    URecordManagerSubsystem* RecordManager = GameInstance->GetSubsystem<URecordManagerSubsystem>();
    UFinanceManagerSubsystem* FinanceManager = GameInstance->GetSubsystem<UFinanceManagerSubsystem>();
    UMarketManagerSubsystem* MarketManager = GameInstance->GetSubsystem<UMarketManagerSubsystem>();
    UMarketingManagerSubsystem* MarketingManager = GameInstance->GetSubsystem<UMarketingManagerSubsystem>();
    UChartManagerSubsystem* ChartManager = GameInstance->GetSubsystem<UChartManagerSubsystem>();
    UEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UEventSubsystem>();

    if (!LabelSubsystem || !TimeSubsystem || !SongManager || !ArtistManager || !RecordManager || !FinanceManager || !MarketManager || !MarketingManager || !ChartManager || !EventSubsystem)
    {
        UE_LOG(LogMusicSaveSubsystem, Error, TEXT("ApplyValidatedSave failed: required subsystem is unavailable."));
        return false;
    }

    LabelSubsystem->ApplySaveSnapshot(SaveObject.PlayerLabelSnapshot);
    TimeSubsystem->ApplySaveSnapshot(SaveObject.TimeSnapshot);
    SongManager->DeserializeFromSave(SaveObject.SavedSongs);
    ArtistManager->ApplySaveSnapshot(SaveObject.ArtistSnapshot);
    RecordManager->ApplySaveSnapshot(SaveObject.RecordSnapshot);
    FinanceManager->ApplySaveSnapshot(SaveObject.FinanceSnapshot);
    MarketManager->ApplySaveSnapshot(SaveObject.MarketSnapshot);
    MarketingManager->ApplySaveSnapshot(SaveObject.MarketingSnapshot);
    ChartManager->ApplySaveSnapshot(SaveObject.ChartSnapshot);
    EventSubsystem->ApplySaveSnapshot(SaveObject.NewsSnapshot);

    if (UUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UUIManagerSubsystem>())
    {
        UIManager->SetCurrentLabelId(SaveObject.PlayerLabelSnapshot.LabelId);
        UIManager->RebuildUI();
    }

    return true;
}

void UMusicSaveSubsystem::LogValidationResult(const FMusicSaveValidationResult& Result) const
{
    for (const FString& Warning : Result.Warnings)
    {
        UE_LOG(LogMusicSaveSubsystem, Warning, TEXT("Save validation warning: %s"), *Warning);
    }

    for (const FString& Error : Result.Errors)
    {
        UE_LOG(LogMusicSaveSubsystem, Error, TEXT("Save validation error: %s"), *Error);
    }
}

bool UMusicSaveSubsystem::WriteToSlot(UMusicSaveGame* SaveObject, const FString& SlotName)
{
    if (!ensure(IsInGameThread()))
    {
        return false;
    }

    if (!SaveObject)
    {
        return false;
    }

    return WriteRawSaveObjectToSlot(SaveObject, SlotName);
}

UMusicSaveGame* UMusicSaveSubsystem::ReadFromSlot(const FString& SlotName)
{
    if (!ensure(IsInGameThread()))
    {
        return nullptr;
    }

    UObject* RawObject = UGameplayStatics::LoadGameFromSlot(SlotName, 0);
    return Cast<UMusicSaveGame>(RawObject);
}

UMusicSaveSlotRegistry* UMusicSaveSubsystem::LoadSlotRegistry() const
{
    UObject* RawObject = UGameplayStatics::LoadGameFromSlot(SlotRegistryName, 0);
    if (UMusicSaveSlotRegistry* Registry = Cast<UMusicSaveSlotRegistry>(RawObject))
    {
        return Registry;
    }

    return NewObject<UMusicSaveSlotRegistry>();
}

bool UMusicSaveSubsystem::WriteSlotRegistry(UMusicSaveSlotRegistry* Registry) const
{
    if (!Registry)
    {
        return false;
    }

    return WriteRawSaveObjectToSlot(Registry, SlotRegistryName);
}

void UMusicSaveSubsystem::RegisterSlotDescriptor(const FSaveSlotDescriptor& Descriptor)
{
    UMusicSaveSlotRegistry* Registry = LoadSlotRegistry();
    if (!Registry)
    {
        return;
    }

    Registry->Registry.Slots.RemoveAll([&Descriptor](const FSaveSlotDescriptor& Existing)
    {
        return Existing.SlotName == Descriptor.SlotName;
    });
    Registry->Registry.Slots.Add(Descriptor);
    WriteSlotRegistry(Registry);
}

void UMusicSaveSubsystem::CreateBackupForExistingSlot(const FString& SlotName)
{
    if (SlotName.IsEmpty() || SlotName == AutoSaveSlotName || !UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        return;
    }

    UMusicSaveGame* ExistingSave = ReadFromSlot(SlotName);
    if (!ExistingSave)
    {
        return;
    }

    const FString BackupSlotName = BuildBackupSlotName(SlotName);
    if (WriteToSlot(ExistingSave, BackupSlotName))
    {
        RegisterSlotDescriptor(ExistingSave->BuildSlotDescriptor(BackupSlotName, false, true, SlotName));
        UE_LOG(LogMusicSaveSubsystem, Log, TEXT("Created backup '%s' before overwriting slot '%s'."), *BackupSlotName, *SlotName);
    }
}

FString UMusicSaveSubsystem::BuildBackupSlotName(const FString& SlotName) const
{
    return FString::Printf(TEXT("%s_Backup_%s"), *SlotName, *FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")));
}

bool UMusicSaveSubsystem::WriteRawSaveObjectToSlot(USaveGame* SaveObject, const FString& SlotName) const
{
    if (!ensure(IsInGameThread()) || !SaveObject || SlotName.IsEmpty())
    {
        return false;
    }

    return UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
}
