// File: Private/EventTickerWidget.cpp
#include "EventTickerWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

UEventTickerWidget::UEventTickerWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UEventTickerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Refresh();
}

void UEventTickerWidget::SetNewsEvent(const FMusicNewsEvent& InEvent)
{
    NewsEvent = InEvent;
    Refresh();
}

void UEventTickerWidget::Refresh_Implementation()
{
    if (IsValid(HeadlineText))
    {
        HeadlineText->SetText(NewsEvent.Headline.IsEmpty() ? FText::GetEmpty() : FText::FromString(NewsEvent.Headline));
    }

    if (IsValid(SourceText))
    {
        SourceText->SetText(NewsEvent.SourceName.IsEmpty() ? FText::GetEmpty() : FText::FromString(NewsEvent.SourceName));
    }

    if (IsValid(TimestampText))
    {
        const FString TimestampString = NewsEvent.Timestamp.ToString(TEXT("%b %d, %Y"));
        TimestampText->SetText(FText::FromString(TimestampString));
    }

    if (IsValid(SubjectText))
    {
        if (NewsEvent.SubjectName.IsEmpty())
        {
            SubjectText->SetText(FText::GetEmpty());
            SubjectText->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            SubjectText->SetText(FText::FromString(NewsEvent.SubjectName));
            SubjectText->SetVisibility(ESlateVisibility::Visible);
        }
    }

    if (IsValid(BodyText))
    {
        BodyText->SetText(NewsEvent.BodyText.IsEmpty() ? FText::GetEmpty() : FText::FromString(NewsEvent.BodyText));
    }

    if (IsValid(TagContainer))
    {
        TagContainer->ClearChildren();

        for (const FString& Tag : NewsEvent.Tags)
        {
            if (Tag.IsEmpty())
            {
                continue;
            }

            UTextBlock* TagText = NewObject<UTextBlock>(TagContainer);
            if (!IsValid(TagText))
            {
                continue;
            }

            TagText->SetText(FText::FromString(Tag));
            TagContainer->AddChildToHorizontalBox(TagText);
        }
    }

    if (IsValid(CategoryIcon))
    {
        FLinearColor CategoryColor = FLinearColor::White;
        ESlateVisibility DesiredVisibility = ESlateVisibility::Visible;

        switch (NewsEvent.NewsType)
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

        CategoryIcon->SetVisibility(DesiredVisibility);
        CategoryIcon->SetColorAndOpacity(CategoryColor);
    }
}
