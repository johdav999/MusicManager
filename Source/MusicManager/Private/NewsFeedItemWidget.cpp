// File: Private/NewsFeedItemWidget.cpp
#include "NewsFeedItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "NewsFeedList.h"
#include "Blueprint/WidgetTree.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Styling/SlateBrush.h"

DEFINE_LOG_CATEGORY_STATIC(LogNewsFeedItemWidget, Log, All);

UNewsFeedItemWidget::UNewsFeedItemWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UNewsFeedItemWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ResolveWidgetBindings();

    UE_LOG(LogNewsFeedItemWidget, Warning, TEXT("NewsFeedItem constructed: Widget=%s HeadlineText=%s NewsTypeIcon=%s Visibility=%d HasCachedEvent=%s."),
        *GetNameSafe(this),
        *GetNameSafe(HeadlineText),
        *GetNameSafe(NewsTypeIcon),
        static_cast<int32>(GetVisibility()),
        bHasCachedEvent ? TEXT("true") : TEXT("false"));

    if (bHasCachedEvent)
    {
        ApplyCachedEventToWidgets();
    }
}

void UNewsFeedItemWidget::ResolveWidgetBindings()
{
    if (!HeadlineText && WidgetTree)
    {
        if (UWidget* FoundWidget = WidgetTree->FindWidget(TEXT("HeadlineText")))
        {
            HeadlineText = Cast<UTextBlock>(FoundWidget);
        }
    }

    if (!NewsTypeIcon && WidgetTree)
    {
        if (UWidget* FoundWidget = WidgetTree->FindWidget(TEXT("NewsTypeIcon")))
        {
            NewsTypeIcon = Cast<UImage>(FoundWidget);
        }
    }

    if (!SourceText && WidgetTree)
    {
        if (UWidget* FoundWidget = WidgetTree->FindWidget(TEXT("SourceText")))
        {
            SourceText = Cast<UTextBlock>(FoundWidget);
        }
    }

    if (!DateText && WidgetTree)
    {
        if (UWidget* FoundWidget = WidgetTree->FindWidget(TEXT("DateText")))
        {
            DateText = Cast<UTextBlock>(FoundWidget);
        }
    }

    if (!DateIcon && WidgetTree)
    {
        if (UWidget* FoundWidget = WidgetTree->FindWidget(TEXT("DateIcon")))
        {
            DateIcon = Cast<UImage>(FoundWidget);
        }
    }

    if (!AccentDivider && WidgetTree)
    {
        if (UWidget* FoundWidget = WidgetTree->FindWidget(TEXT("AccentDivider")))
        {
            AccentDivider = Cast<UImage>(FoundWidget);
        }
    }
}

void UNewsFeedItemWidget::SetupFromEvent(const FMusicNewsEvent& Event)
{
    CachedEvent = Event;
    bHasCachedEvent = true;
    ResolveWidgetBindings();
    ApplyCachedEventToWidgets();
}

void UNewsFeedItemWidget::ApplyCachedEventToWidgets()
{
    SetVisibility(ESlateVisibility::Visible);
    SetRenderOpacity(1.f);

    if (IsValid(HeadlineText))
    {
        HeadlineText->SetText(CachedEvent.Headline.IsEmpty() ? FText::GetEmpty() : FText::FromString(CachedEvent.Headline));
        HeadlineText->SetVisibility(ESlateVisibility::Visible);
        HeadlineText->SetRenderOpacity(1.f);
        UE_LOG(LogNewsFeedItemWidget, Warning, TEXT("NewsFeedItem headline applied: Widget=%s Headline='%s'."),
            *GetNameSafe(this),
            *CachedEvent.Headline);
    }
    else
    {
        UE_LOG(LogNewsFeedItemWidget, Error, TEXT("NewsFeedItem missing HeadlineText binding: Widget=%s Headline='%s'. Check NewsFeedItemBP has a TextBlock named HeadlineText or bound to HeadlineText."),
            *GetNameSafe(this),
            *CachedEvent.Headline);
    }

    if (IsValid(SourceText))
    {
        SourceText->SetText(FText::FromString(ResolveSourceText()));
        SourceText->SetVisibility(ESlateVisibility::Visible);
    }

    if (IsValid(DateText))
    {
        DateText->SetText(FormatNewsDate(CachedEvent.Timestamp));
        DateText->SetVisibility(ESlateVisibility::Visible);
    }

    if (IsValid(DateIcon))
    {
        DateIcon->SetVisibility(ESlateVisibility::Visible);
        DateIcon->SetColorAndOpacity(FLinearColor(1.f, 0.827f, 0.353f, 1.f));
    }

    if (IsValid(AccentDivider))
    {
        AccentDivider->SetVisibility(ESlateVisibility::Visible);
        AccentDivider->SetColorAndOpacity(FLinearColor(1.f, 0.827f, 0.353f, 1.f));
    }

    ApplyNewsTypeIcon(CachedEvent.NewsType);
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

    ESlateVisibility DesiredVisibility = ESlateVisibility::Visible;
    if (UTexture2D* Texture = ResolveNewsTypeTexture(NewsType))
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(Texture);
        Brush.ImageSize = FVector2D(42.f, 42.f);
        NewsTypeIcon->SetBrush(Brush);
    }

    switch (NewsType)
    {
    case EMusicNewsType::None:
        DesiredVisibility = ESlateVisibility::Collapsed;
        break;
    default:
        break;
    }

    NewsTypeIcon->SetVisibility(DesiredVisibility);
    NewsTypeIcon->SetColorAndOpacity(FLinearColor::White);
}

FString UNewsFeedItemWidget::ResolveSourceText() const
{
    if (!CachedEvent.SourceName.IsEmpty())
    {
        return CachedEvent.SourceName;
    }

    switch (CachedEvent.NewsType)
    {
    case EMusicNewsType::ArtistPerformance:
    case EMusicNewsType::NewUpcomingArtistPerforming:
    case EMusicNewsType::FestivalAnnouncement:
        return TEXT("City Herald");
    case EMusicNewsType::DealSigned:
    case EMusicNewsType::Partnership:
        return TEXT("A&R Desk");
    case EMusicNewsType::ChartAchievement:
    case EMusicNewsType::IndustryTrend:
    case EMusicNewsType::MarketShift:
        return TEXT("Market Watch");
    case EMusicNewsType::MarketingPush:
        return TEXT("Radio Weekly");
    default:
        return TEXT("MusicManager News");
    }
}

FText UNewsFeedItemWidget::FormatNewsDate(const FDateTime& Timestamp)
{
    const FDateTime Date = Timestamp.GetTicks() > 0 ? Timestamp : FDateTime(1955, 1, 1);
    static const FString MonthNames[12] = {
        TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"),
        TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec")
    };
    const int32 Month = Date.GetMonth();
    const FString MonthName = (Month >= 1 && Month <= 12) ? MonthNames[Month - 1] : TEXT("Jan");
    return FText::FromString(FString::Printf(TEXT("%s %d, %d"), *MonthName, Date.GetDay(), Date.GetYear()));
}

UTexture2D* UNewsFeedItemWidget::ResolveNewsTypeTexture(EMusicNewsType NewsType)
{
    const TCHAR* Path = TEXT("/Game/GUI/News/NewsFeedIcon_Microphone.NewsFeedIcon_Microphone");
    switch (NewsType)
    {
    case EMusicNewsType::DealSigned:
    case EMusicNewsType::Partnership:
        Path = TEXT("/Game/GUI/News/NewsFeedIcon_Handshake.NewsFeedIcon_Handshake");
        break;
    case EMusicNewsType::MarketingPush:
    case EMusicNewsType::IndustryTrend:
        Path = TEXT("/Game/GUI/News/NewsFeedIcon_Radio.NewsFeedIcon_Radio");
        break;
    case EMusicNewsType::ChartAchievement:
    case EMusicNewsType::FinancialReport:
    case EMusicNewsType::MarketShift:
        Path = TEXT("/Game/GUI/News/NewsFeedIcon_Chart.NewsFeedIcon_Chart");
        break;
    default:
        break;
    }

    return LoadObject<UTexture2D>(nullptr, Path);
}
