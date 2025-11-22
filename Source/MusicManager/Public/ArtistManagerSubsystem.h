#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FArtistContract.h"
#include "ArtistManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArtistSigned, const FArtistContract&, SignedContract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArtistRejected, const FString&, ArtistId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContractExpired, const FArtistContract&, ExpiredContract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContractsUpdated, const TArray<FArtistContract>&, UpdatedContracts);

class UMusicSaveGame;

UCLASS()
class UArtistManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void SignArtist(const FArtistDealTerms& Deal, const FArtistData& ArtistInfo);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void RejectArtist(const FString& ArtistId);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void AdvanceMonth();

    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);

    void ProcessMonthlyContractFinancials(FArtistContract& Contract);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void ExpireContract(const FString& ArtistId);

    const FArtistContract* GetContractByArtistId(const FString& ArtistId) const;

    void GetSignedArtistData(TArray<FArtistData>& OutArtistData) const;

    const FArtistContract* FindContractByArtistName(const FString& ArtistName) const;

    void SaveState(class UMusicSaveGame* SaveObject);
    void LoadState(const class UMusicSaveGame* SaveObject);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contracts")
    TArray<FArtistContract> ActiveContracts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contracts")
    TArray<FArtistContract> ExpiredContracts;

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

protected:
    int32 CalculateContractDurationMonths(const FArtistDealTerms& Deal) const;
};
