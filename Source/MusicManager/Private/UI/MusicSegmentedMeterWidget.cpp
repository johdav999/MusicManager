#include "UI/MusicSegmentedMeterWidget.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

class SMusicSegmentedMeter : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SMusicSegmentedMeter) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs) {}

    void SetPercent(float InPercent)
    {
        Percent = FMath::Clamp(InPercent, 0.f, 1.f);
    }

    void SetSegmentCount(int32 InSegmentCount)
    {
        SegmentCount = FMath::Clamp(InSegmentCount, 1, 24);
    }

    void SetColors(const FLinearColor& InFilled, const FLinearColor& InEmpty, const FLinearColor& InOutline)
    {
        FilledColor = InFilled;
        EmptyColor = InEmpty;
        OutlineColor = InOutline;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(238.f, 22.f);
    }

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        constexpr float Gap = 4.f;
        const float SegmentWidth = FMath::Max(1.f, (Size.X - Gap * static_cast<float>(SegmentCount - 1)) / static_cast<float>(SegmentCount));
        const float FilledSegmentsFloat = Percent * static_cast<float>(SegmentCount);

        const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
        for (int32 Index = 0; Index < SegmentCount; ++Index)
        {
            const float X = static_cast<float>(Index) * (SegmentWidth + Gap);
            const FVector2D Pos(X, 0.f);
            const FVector2D SegmentSize(SegmentWidth, Size.Y);

            FSlateDrawElement::MakeBox(
                OutDrawElements,
                LayerId,
                AllottedGeometry.ToPaintGeometry(Pos, SegmentSize),
                Brush,
                ESlateDrawEffect::None,
                OutlineColor);

            const bool bFilled = static_cast<float>(Index) < FilledSegmentsFloat;
            const FLinearColor Fill = bFilled ? FilledColor : EmptyColor;
            FSlateDrawElement::MakeBox(
                OutDrawElements,
                LayerId + 1,
                AllottedGeometry.ToPaintGeometry(Pos + FVector2D(1.f, 1.f), SegmentSize - FVector2D(2.f, 2.f)),
                Brush,
                ESlateDrawEffect::None,
                Fill);
        }

        return LayerId + 1;
    }

private:
    float Percent = 0.f;
    int32 SegmentCount = 12;
    FLinearColor FilledColor = FLinearColor(1.f, 0.827f, 0.353f, 1.f);
    FLinearColor EmptyColor = FLinearColor(0.13f, 0.125f, 0.105f, 0.95f);
    FLinearColor OutlineColor = FLinearColor(0.48f, 0.32f, 0.09f, 0.85f);
};

void UMusicSegmentedMeterWidget::SetPercent(float InPercent)
{
    Percent = FMath::Clamp(InPercent, 0.f, 1.f);
    if (MeterWidget)
    {
        MeterWidget->SetPercent(Percent);
    }
}

TSharedRef<SWidget> UMusicSegmentedMeterWidget::RebuildWidget()
{
    MeterWidget = SNew(SMusicSegmentedMeter);
    SynchronizeProperties();
    return MeterWidget.ToSharedRef();
}

void UMusicSegmentedMeterWidget::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (MeterWidget)
    {
        MeterWidget->SetPercent(Percent);
        MeterWidget->SetSegmentCount(SegmentCount);
        MeterWidget->SetColors(FilledColor, EmptyColor, OutlineColor);
    }
}

void UMusicSegmentedMeterWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    MeterWidget.Reset();
}
