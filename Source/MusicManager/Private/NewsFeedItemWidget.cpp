// File: Private/NewsFeedItemWidget.cpp
#include "NewsFeedItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "NewsFeedList.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

UNewsFeedItemWidget::UNewsFeedItemWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UNewsFeedItemWidget::SetupFromEvent(const FMusicNewsEvent& Event)
{
    CachedEvent = Event;
    if (HeadlineText)
    {
        HeadlineText->SetText(Event.Headline.IsEmpty() ? FText::GetEmpty() : FText::FromString(Event.Headline));
    }

    ApplyNewsTypeIcon(Event.NewsType);
}

const FMusicNewsEvent& UNewsFeedItemWidget::GetNewsEvent() const
{
    return CachedEvent;
}

void UNewsFeedItemWidget::SetOwnerList(UNewsFeedList* InOwnerList)
{
    OwnerList = InOwnerList;
}

void UNewsFeedItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (UNewsFeedList* List = OwnerList.Get())
    {
        List->HandleItemHovered(this);
    }
}

void UNewsFeedItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (UNewsFeedList* List = OwnerList.Get())
    {
        List->HandleItemUnhovered(this);
    }
}

FReply UNewsFeedItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    {
        if (UNewsFeedList* List = OwnerList.Get())
        {
            List->HandleItemToggled(this);
        }

        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UNewsFeedItemWidget::ApplyNewsTypeIcon(EMusicNewsType NewsType)
{
    if (!NewsTypeIcon)
    {
        return;
    }

    FLinearColor CategoryColor = FLinearColor::White;
    ESlateVisibility DesiredVisibility = ESlateVisibility::Visible;

    switch (NewsType)
    {
    case EMusicNewsType::None:
        DesiredVisibility = ESlateVisibility::Collapsed;
        break;
    case EMusicNewsType::ArtistSigned:
    case EMusicNewsType::ArtistAward:
        CategoryColor = FLinearColor(0.18f, 0.6f, 0.27f, 1.0f);
        break;
    case EMusicNewsType::ArtistDropped:
    case EMusicNewsType::ArtistScandal:
        CategoryColor = FLinearColor(0.75f, 0.19f, 0.19f, 1.0f);
        break;
    case EMusicNewsType::ArtistPerformance:
    case EMusicNewsType::NewUpcomingArtistPerforming:
    case EMusicNewsType::FestivalAnnouncement:
        CategoryColor = FLinearColor(0.26f, 0.41f, 0.85f, 1.0f);
        break;
    case EMusicNewsType::RecordRelease:
    case EMusicNewsType::MusicVideoRelease:
    case EMusicNewsType::RecordingSession:
        CategoryColor = FLinearColor(0.82f, 0.46f, 0.12f, 1.0f);
        break;
    case EMusicNewsType::ChartAchievement:
    case EMusicNewsType::IndustryTrend:
        CategoryColor = FLinearColor(0.56f, 0.3f, 0.85f, 1.0f);
        break;
    case EMusicNewsType::DealSigned:
    case EMusicNewsType::Partnership:
    case EMusicNewsType::MarketingPush:
        CategoryColor = FLinearColor(0.11f, 0.65f, 0.71f, 1.0f);
        break;
    case EMusicNewsType::FinancialReport:
    case EMusicNewsType::LabelExpansion:
        CategoryColor = FLinearColor(0.95f, 0.76f, 0.16f, 1.0f);
        break;
    case EMusicNewsType::RivalLabelNews:
    case EMusicNewsType::MarketShift:
        CategoryColor = FLinearColor(0.47f, 0.47f, 0.47f, 1.0f);
        break;
    default:
        break;
    }

    NewsTypeIcon->SetVisibility(DesiredVisibility);
    NewsTypeIcon->SetColorAndOpacity(CategoryColor);
}
