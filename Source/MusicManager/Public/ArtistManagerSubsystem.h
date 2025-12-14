#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FArtistContract.h"
#include "Engine/DataTable.h"
#include "ArtistManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArtistSigned, const FArtistContract&, SignedContract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArtistRejected, const FString&, ArtistId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContractExpired, const FArtistContract&, ExpiredContract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContractsUpdated, const TArray<FArtistContract>&, UpdatedContracts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArtistListChanged);

class UMusicSaveGame;
class USong;
struct FSongData;

USTRUCT(BlueprintType)
struct FArtistSongList
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FString> SongIds;
};

  /**
   * Read-only view of per-market artist modifiers used by the record sales simulator.
   */
  USTRUCT(BlueprintType)
  struct FArtistMarketModifiers
  {
      GENERATED_BODY();

      UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artist")
      float Popularity = 1.0f;

      UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artist")
      float Momentum = 1.0f;

      UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artist")
      float Reputation = 1.0f;

      UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artist")
      float GenreAlignment = 1.0f;

      UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artist")
      float Cannibalization = 1.0f;

      float GetComposite() const
      {
          return Popularity * Momentum * Reputation * GenreAlignment * Cannibalization;
      }
  };

  UCLASS()
  class UArtistManagerSubsystem : public UGameInstanceSubsystem
  {
      GENERATED_BODY()

public:
    UArtistManagerSubsystem();
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Artists")
    void GetUnsignedArtists(TArray<FArtistData>& OutArtists) const;

    UFUNCTION(BlueprintCallable, Category="Artists")
    bool GetNextUnsignedArtist(FArtistData& OutArtist) const;

    UFUNCTION(BlueprintCallable, Category="Artists")
    void RotateUnsignedArtist();

    USong* CreateSongForArtist(const FString& ArtistId, const FSongData& Data);

    void GetSongsForArtist(const FString& ArtistId, TArray<USong*>& OutSongs) const;

    void RegisterSongToArtist(const FString& ArtistId, const FString& SongId);

    UFUNCTION(BlueprintCallable, Category="Artists")
    void LoadArtistsFromDataTable();

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void SignArtist(const FArtistDealTerms& Deal);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void RejectArtist(const FString& ArtistId);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void AdvanceMonth();

    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);

    void ProcessMonthlyContractFinancials(FArtistContract& Contract);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void ExpireContract(const FString& ArtistId);

    /** Tracks the currently selected artist (globally used by UI systems). */
    UPROPERTY()
    FString SelectedArtistId;

    UFUNCTION()
    void SetSelectedArtist(const FString& ArtistId);

    UFUNCTION()
    FString GetSelectedArtist() const;

    const FArtistContract* GetContractByArtistId(const FString& ArtistId) const;

    void GetSignedArtistData(TArray<FArtistData>& OutArtistData) const;

    const FArtistContract* FindContractByArtistName(const FString& ArtistName) const;

    /** Build per-market artist multipliers for the sales simulator. */
    FArtistMarketModifiers EvaluateMarketModifiers(const FString& ArtistId, const FString& Genre, const FString& MarketId, const FDateTime& CurrentDate, int32 ConcurrentReleases) const;

    /** Apply monthly decay/boost to stored momentum values. */
    void ApplyMonthlyMomentum();

    /** Calculate a cannibalization factor when an artist releases multiple records simultaneously. */
    float CalculateCannibalization(const FString& ArtistId, int32 ConcurrentReleases) const;

    void SaveState(class UMusicSaveGame* SaveObject);
    void LoadState(const class UMusicSaveGame* SaveObject);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contracts")
    TArray<FArtistContract> ActiveContracts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contracts")
    TArray<FArtistContract> ExpiredContracts;

    UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Artists")
    UDataTable* ArtistDataTable;

    UPROPERTY(VisibleAnywhere, Category="Artists")
    TArray<FArtistData> UnsignedArtists;

    UPROPERTY()
    TMap<FString, FArtistSongList> ArtistToSongs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contracts")
    FDateTime CurrentGameDate = FDateTime();

    UPROPERTY(BlueprintAssignable, Category="Contracts")
    FOnArtistSigned OnArtistSigned;

    UPROPERTY(BlueprintAssignable, Category="Contracts")
    FOnArtistRejected OnArtistRejected;

    UPROPERTY(BlueprintAssignable, Category="Contracts")
    FOnContractExpired OnContractExpired;

    UPROPERTY(BlueprintAssignable, Category="Contracts")
    FOnContractsUpdated OnMonthlyFinancialUpdate;

    UPROPERTY(BlueprintAssignable, Category="Contracts")
    FOnArtistListChanged OnArtistListChanged;

    /** Runtime momentum tracking per artist. */
    UPROPERTY()
    TMap<FString, float> ArtistMomentum;

    /** Simplified reputation cache per artist (driven by contract history). */
    UPROPERTY()
    TMap<FString, float> ArtistReputation;

protected:
    int32 CalculateContractDurationMonths(const FArtistDealTerms& Deal) const;
};
