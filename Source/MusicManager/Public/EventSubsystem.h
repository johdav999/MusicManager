// File: Public/EventSubsystem.h
#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "EventSubsystem.generated.h"

class ULayout;
class UUserWidget;
class UWorld;

DECLARE_LOG_CATEGORY_EXTERN(LogEventSubsystem, Log, All);


UENUM(BlueprintType)
enum class EMusicNewsType : uint8
{
    None                UMETA(DisplayName = "None"),

    // --- Artist & career ---
    ArtistSigned        UMETA(DisplayName = "Artist Signed to Label"),
    ArtistDropped       UMETA(DisplayName = "Artist Dropped from Label"),
    ArtistPerformance   UMETA(DisplayName = "Live Performance"),
    ArtistAward         UMETA(DisplayName = "Artist Wins Award"),
    ArtistScandal       UMETA(DisplayName = "Artist Scandal or Controversy"),

    // --- Releases & production ---
    RecordRelease       UMETA(DisplayName = "Record Release"),
    MusicVideoRelease   UMETA(DisplayName = "Music Video Release"),
    RecordingSession    UMETA(DisplayName = "Recording Session Started/Ended"),
    ChartAchievement    UMETA(DisplayName = "Chart Achievement"),

    // --- Business & industry ---
    DealSigned          UMETA(DisplayName = "New Business Deal"),
    MarketingPush       UMETA(DisplayName = "Marketing Campaign Launch"),
    Partnership         UMETA(DisplayName = "Partnership or Collaboration"),
    FinancialReport     UMETA(DisplayName = "Financial Report or Milestone"),
    LabelExpansion      UMETA(DisplayName = "Label Expansion / New Office"),

    // --- World & culture ---
    FestivalAnnouncement UMETA(DisplayName = "Festival or Event Announcement"),
    IndustryTrend        UMETA(DisplayName = "Industry Trend"),
    RivalLabelNews       UMETA(DisplayName = "Rival Label News"),
    MarketShift          UMETA(DisplayName = "Market Shift"),
};

USTRUCT(BlueprintType)
struct FMusicNewsEvent
{
    GENERATED_BODY()

    /** Unique ID for saving or sorting */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid NewsId;

    /** When this news occurs (in-game time) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime Timestamp;

    /** Type/category of news */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMusicNewsType NewsType = EMusicNewsType::None;

    /** Source of the news (artist, label, festival, etc.) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SourceName;

    /** Optional subject: album, tour, award, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SubjectName;

    /** Short headline for newsfeed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Headline;

    /** Body text for details panel */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
    FString BodyText;

    /** Tags for filtering/searching (e.g. "Rock", "Europe", "Scandal") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> Tags;

    /** Impact on gameplay systems */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ReputationDelta = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RevenueDelta = 0;

    /** Optional data for UI or logic hooks */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> Metadata;
};


/**
 * Game-instance subsystem that periodically notifies a child widget on a registered layout widget.
 */
UCLASS(Config=Game)
class UEventSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="EventSubsystem")
    void RegisterLayout(ULayout* InLayout);

    UFUNCTION(BlueprintCallable, Category="EventSubsystem")
    void UnregisterLayout(ULayout* InLayout);


    void HandlePostWorldInit(UWorld* InWorld, const UWorld::InitializationValues IVS);
    void SendDummyNews();
    void HandleWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources);
    void StartTimerForWorld(UWorld* InWorld);
    void StopTimer();
    void OnTimerTick();
    UUserWidget* ResolveChildWidget(ULayout& Layout);
    bool IsSameGameInstanceWorld(const UWorld& World) const;

    TWeakObjectPtr<UWorld> CachedWorld;
    FTimerHandle EventTimerHandle;
    FDelegateHandle WorldInitHandle;
    FDelegateHandle WorldCleanupHandle;

    UPROPERTY(EditAnywhere, Config, meta=(ClampMin="0.1", ClampMax="60.0"))
    float TickIntervalSeconds = 5.0f;

    UPROPERTY(EditAnywhere, Config)
    FName ChildWidgetName = TEXT("EventTicker");

    UPROPERTY(EditAnywhere,BlueprintReadWrite, Config)
    TSubclassOf<UUserWidget> ChildWidgetClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<ULayout> LayoutWeak;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<UUserWidget> ChildWeak;
};
