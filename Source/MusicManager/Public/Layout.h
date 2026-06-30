// File: Public/Layout.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "ContractWidget.h"
#include "EventTickerWidget.h"
#include "NewsFeedItemWidget.h"
#include "UI/RecordWidget.h"
#include "AuditionTypes.h"
#include "UI/TooltipData.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Layout.generated.h"

class UNewsFeedList;
class UUserWidget;
class UAuditionWidget;
class UHoverTooltipManagerWidget;
class UInspectorPanelWidget;
class UMainCanvasHost;
class UUIManagerSubsystem;
class USignedArtistPanelWidget;
class UArtistManagerSubsystem;
class URegionMapWidget;
class UArtistHoverDetailWidget;
class UTopStatusBarWidget;
class UActiveContractsWidget;

/**
 * Layout widget that exposes helpers for locating child widgets by name or class.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewsCardSelected, UEventTickerWidget*, SelectedCard);

UCLASS(BlueprintType, Blueprintable)
class ULayout final : public UUserWidget
{
    GENERATED_BODY()

public:
    ULayout(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Locate a user widget child by name or class, preferring the name when provided. */
    UFUNCTION(BlueprintCallable, Category="EventSubsystem")
    UUserWidget* GetChildByNameOrClass(FName WidgetName, TSubclassOf<UUserWidget> WidgetClass) const;

    UFUNCTION(BlueprintCallable, Category="News")
    void AddNewsCardToFeed(const FMusicNewsEvent& Event);

    UFUNCTION(BlueprintCallable, Category="News")
    void RemoveNewsCardFromFeed(UNewsFeedItemWidget* Card);

    /** Raised when any card in the feed is clicked */
    UPROPERTY(BlueprintAssignable, Category="News")
    FOnNewsCardSelected OnNewsCardSelected;

    /** Called when a ticker widget is created and added to the feed */
    UFUNCTION(BlueprintCallable, Category="News")
    void BindTickerEvents(UNewsFeedItemWidget* NewItem);

    UFUNCTION(BlueprintCallable, Category="Layout")
    void ShowAuditionWidget();

    UFUNCTION(BlueprintCallable, Category="Layout")
    void ShowAuditionWidgetForArtist(const FArtistData& ArtistData);

    UFUNCTION(BlueprintCallable, Category = "UI")
    void CloseAuditionWidget();

    UFUNCTION(BlueprintCallable, Category="Studio")
    void ShowRecordWidget();

    UFUNCTION(BlueprintCallable, Category="Studio")
    void CloseRecordWidget();

    /** Show the audition widget and populate it with real audition data from an FAuditionEvent. */
    UFUNCTION(BlueprintCallable, Category = "Audition")
    void ShowAuditionWidgetWithData(const FAuditionEvent& EventData);

    UFUNCTION(BlueprintCallable)
    void ShowRegionMap();

    UFUNCTION(BlueprintCallable, Category = "Contract")
    void ShowContract(const FArtistContract& SignedContract);

    UFUNCTION(BlueprintCallable, Category = "Contract")
    void ShowActiveContractsWidget();

    UFUNCTION(BlueprintCallable, Category = "Contract")
    void ShowActiveContractsWidgetForArtist(const FString& ArtistId);

    UFUNCTION(BlueprintCallable, Category = "Contract")
    void CloseActiveContractsWidget();

    UFUNCTION(BlueprintCallable, Category="Artist")
    void RefreshSignedArtists(const TArray<FArtistData>& Artists);

    UFUNCTION(BlueprintCallable, Category="Layout")
    UAuditionWidget* GetAuditionWidget() const;

    UFUNCTION(BlueprintCallable, Category="Layout")
    UMainCanvasHost* GetMainCanvasHost() const { return MainCanvasHost; }

    UFUNCTION(BlueprintCallable, Category="Layout")
    UInspectorPanelWidget* GetInspectorPanel() const { return InspectorPanelWidget; }

    UFUNCTION(BlueprintCallable, Category="Layout")
    UTopStatusBarWidget* GetTopStatusBarWidget() const { return TopStatusBarWidget; }

    /** Layer-2 hover tooltip routing (UI manager only). */
    void ShowHoverTooltip(const FTooltipData& Data);
    void HideHoverTooltip();
    void ShowArtistHoverDetail(const FArtistData& ArtistData, const FVector2D& ScreenPosition);
    void HideArtistHoverDetail();

    /** Toggle the layer-2 root to enforce hover-only interactions. */
    void SetLayer2Enabled(bool bEnabled);

protected:
    UPROPERTY(meta=(BindWidgetOptional))
    UWidget* Layer1_Root;

    UPROPERTY(meta=(BindWidgetOptional))
    UTopStatusBarWidget* TopStatusBarWidget;

    UPROPERTY(EditDefaultsOnly, Category="Layout|HUD")
    TSubclassOf<UTopStatusBarWidget> TopStatusBarWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="Layout|HUD")
    TSubclassOf<UAuditionWidget> ArtistAuditionPanelWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="Layout|Studio")
    TSubclassOf<URecordWidget> RecordWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="Layout|Contracts")
    TSubclassOf<UActiveContractsWidget> ActiveContractsWidgetClass;

    UPROPERTY(meta=(BindWidgetOptional))
    UWidget* Layer2_Root;

    UPROPERTY(meta=(BindWidgetOptional))
    UInspectorPanelWidget* InspectorPanelWidget;

    UPROPERTY(meta=(BindWidgetOptional))
    UMainCanvasHost* MainCanvasHost;

    UPROPERTY(meta=(BindWidgetOptional))
    UHoverTooltipManagerWidget* HoverTooltipManager;

    UPROPERTY(meta=(BindWidgetOptional))
    UArtistHoverDetailWidget* ArtistHoverDetailWidget;

    UPROPERTY(meta=(BindWidgetOptional))
    UNewsFeedList* NewsFeedList;

    UPROPERTY(Transient)
    UAuditionWidget* AuditionWidget;

    UPROPERTY(meta = (BindWidgetOptional))
        UContractWidget* ContractWidget;

    UPROPERTY(Transient)
    UActiveContractsWidget* ActiveContractsWidget;

    UPROPERTY(meta = (BindWidgetOptional))
    URecordWidget* RecordWidget;

    UPROPERTY(meta=(BindWidget))
    USignedArtistPanelWidget* SignedArtistsPanel;

    UPROPERTY(meta = (BindWidgetOptional))
    URegionMapWidget* RegionMapWidget;

private:
    UFUNCTION()
    void HandleTickerClicked(UEventTickerWidget* ClickedTicker);

    UFUNCTION()
    void HandleNewsFeedItemSelected(UNewsFeedItemWidget* Card, const FMusicNewsEvent& EventData);

    UFUNCTION()
    void HandleArtistSelected(FString ArtistId);

    UFUNCTION()
    void HandleAuditionSigned();

    UFUNCTION()
    void HandleAuditionPassed();

    UFUNCTION()
    void HandleActiveContractsCloseRequested();

    void EnsureTopStatusBarWidget();
    void EnsureArtistAuditionPanelWidget();
    void EnsureRecordWidget();
    void EnsureActiveContractsWidget();

    UUIManagerSubsystem* GetUIManagerSubsystem() const;
    UArtistManagerSubsystem* GetArtistManagerSubsystem() const;
};
