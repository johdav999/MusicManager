#include "UI/MusicGoldSlider.h"

#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace
{
    constexpr float TrackInsetX = 8.f;
    constexpr float TrackHeight = 3.f;
    constexpr float TickHeight = 10.f;
    constexpr float KnobSize = 26.f;
    constexpr int32 TickCount = 7;

    const FLinearColor TrackColor(0.20f, 0.14f, 0.05f, 1.f);
    const FLinearColor FillColor(0.95f, 0.70f, 0.26f, 1.f);
    const FLinearColor TickColor(0.58f, 0.40f, 0.13f, 1.f);
}

class SMusicGoldSlider final : public SLeafWidget
{
public:
    DECLARE_DELEGATE_OneParam(FOnSlateValueChanged, float);

    SLATE_BEGIN_ARGS(SMusicGoldSlider) {}
        SLATE_ARGUMENT(float, Value)
        SLATE_ARGUMENT(float, MinValue)
        SLATE_ARGUMENT(float, MaxValue)
        SLATE_ARGUMENT(float, StepSize)
        SLATE_EVENT(FOnSlateValueChanged, OnValueChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        Value = InArgs._Value;
        MinValue = InArgs._MinValue;
        MaxValue = InArgs._MaxValue;
        StepSize = InArgs._StepSize;
        OnValueChanged = InArgs._OnValueChanged;
        KnobTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/GUI/Audition/ArtistAuditionSliderThumbSurface.ArtistAuditionSliderThumbSurface"));
    }

    void SetRange(float InMinValue, float InMaxValue, float InStepSize)
    {
        MinValue = InMinValue;
        MaxValue = FMath::Max(InMaxValue, MinValue + UE_SMALL_NUMBER);
        StepSize = FMath::Max(0.f, InStepSize);
        SetValue(Value, false);
    }

    void SetValue(float InValue, bool bNotify)
    {
        const float NewValue = SnapValue(FMath::Clamp(InValue, MinValue, MaxValue));
        if (FMath::IsNearlyEqual(Value, NewValue))
        {
            return;
        }

        Value = NewValue;
        if (bNotify && OnValueChanged.IsBound())
        {
            OnValueChanged.Execute(Value);
        }
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(174.f, 30.f);
    }

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
    {
        const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
        const float CenterY = LocalSize.Y * 0.5f;
        const float LeftX = TrackInsetX;
        const float RightX = FMath::Max(LeftX + 1.f, LocalSize.X - TrackInsetX);
        const float TrackWidth = RightX - LeftX;
        const float Percent = GetNormalizedValue();
        const float KnobCenterX = LeftX + TrackWidth * Percent;

        const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2D(LeftX, CenterY - TrackHeight * 0.5f), FVector2D(TrackWidth, TrackHeight)),
            WhiteBrush,
            ESlateDrawEffect::None,
            TrackColor);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2D(LeftX, CenterY - TrackHeight * 0.5f), FVector2D(FMath::Max(1.f, KnobCenterX - LeftX), TrackHeight)),
            WhiteBrush,
            ESlateDrawEffect::None,
            FillColor);

        for (int32 Index = 0; Index <= TickCount; ++Index)
        {
            const float TickX = LeftX + TrackWidth * (static_cast<float>(Index) / static_cast<float>(TickCount));
            FSlateDrawElement::MakeBox(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(FVector2D(TickX - 0.75f, CenterY + 8.f), FVector2D(1.5f, TickHeight)),
                WhiteBrush,
                ESlateDrawEffect::None,
                TickColor);
        }

        FSlateBrush KnobBrush;
        KnobBrush.DrawAs = ESlateBrushDrawType::Image;
        KnobBrush.SetResourceObject(KnobTexture);
        KnobBrush.ImageSize = FVector2D(KnobSize, KnobSize);
        KnobBrush.TintColor = FSlateColor(FLinearColor::White);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 3,
            AllottedGeometry.ToPaintGeometry(FVector2D(KnobCenterX - KnobSize * 0.5f, CenterY - KnobSize * 0.5f), FVector2D(KnobSize, KnobSize)),
            &KnobBrush,
            ESlateDrawEffect::None,
            FLinearColor::White);

        return LayerId + 4;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        {
            return FReply::Unhandled();
        }

        SetValueFromMouse(MyGeometry, MouseEvent);
        return FReply::Handled().CaptureMouse(AsShared());
    }

    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (!HasMouseCapture())
        {
            return FReply::Unhandled();
        }

        SetValueFromMouse(MyGeometry, MouseEvent);
        return FReply::Handled();
    }

    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
        {
            SetValueFromMouse(MyGeometry, MouseEvent);
            return FReply::Handled().ReleaseMouseCapture();
        }

        return FReply::Unhandled();
    }

private:
    float GetNormalizedValue() const
    {
        return FMath::Clamp((Value - MinValue) / FMath::Max(UE_SMALL_NUMBER, MaxValue - MinValue), 0.f, 1.f);
    }

    float SnapValue(float InValue) const
    {
        if (StepSize <= 0.f)
        {
            return InValue;
        }

        const float Steps = FMath::RoundToFloat((InValue - MinValue) / StepSize);
        return FMath::Clamp(MinValue + Steps * StepSize, MinValue, MaxValue);
    }

    void SetValueFromMouse(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    {
        const FVector2D LocalMouse = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        const float LeftX = TrackInsetX;
        const float RightX = FMath::Max(LeftX + 1.f, MyGeometry.GetLocalSize().X - TrackInsetX);
        const float Percent = FMath::Clamp((LocalMouse.X - LeftX) / (RightX - LeftX), 0.f, 1.f);
        SetValue(FMath::Lerp(MinValue, MaxValue, Percent), true);
    }

    float Value = 0.f;
    float MinValue = 0.f;
    float MaxValue = 1.f;
    float StepSize = 0.f;
    UTexture2D* KnobTexture = nullptr;
    FOnSlateValueChanged OnValueChanged;
};

UMusicGoldSlider::UMusicGoldSlider(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMusicGoldSlider::SetValue(float InValue)
{
    const float NewValue = NormalizeValue(InValue);
    if (FMath::IsNearlyEqual(Value, NewValue))
    {
        return;
    }

    Value = NewValue;
    if (SlateSlider.IsValid())
    {
        SlateSlider->SetValue(Value, false);
    }
}

void UMusicGoldSlider::SetMinValue(float InMinValue)
{
    MinValue = InMinValue;
    MaxValue = FMath::Max(MaxValue, MinValue + UE_SMALL_NUMBER);
    Value = NormalizeValue(Value);
    if (SlateSlider.IsValid())
    {
        SlateSlider->SetRange(MinValue, MaxValue, StepSize);
        SlateSlider->SetValue(Value, false);
    }
}

void UMusicGoldSlider::SetMaxValue(float InMaxValue)
{
    MaxValue = FMath::Max(InMaxValue, MinValue + UE_SMALL_NUMBER);
    Value = NormalizeValue(Value);
    if (SlateSlider.IsValid())
    {
        SlateSlider->SetRange(MinValue, MaxValue, StepSize);
        SlateSlider->SetValue(Value, false);
    }
}

void UMusicGoldSlider::SetStepSize(float InStepSize)
{
    StepSize = FMath::Max(0.f, InStepSize);
    Value = NormalizeValue(Value);
    if (SlateSlider.IsValid())
    {
        SlateSlider->SetRange(MinValue, MaxValue, StepSize);
        SlateSlider->SetValue(Value, false);
    }
}

TSharedRef<SWidget> UMusicGoldSlider::RebuildWidget()
{
    SlateSlider = SNew(SMusicGoldSlider)
        .Value(Value)
        .MinValue(MinValue)
        .MaxValue(MaxValue)
        .StepSize(StepSize)
        .OnValueChanged(SMusicGoldSlider::FOnSlateValueChanged::CreateUObject(this, &UMusicGoldSlider::HandleSlateValueChanged));

    return SlateSlider.ToSharedRef();
}

void UMusicGoldSlider::SynchronizeProperties()
{
    Super::SynchronizeProperties();

    Value = NormalizeValue(Value);
    if (SlateSlider.IsValid())
    {
        SlateSlider->SetRange(MinValue, MaxValue, StepSize);
        SlateSlider->SetValue(Value, false);
    }
}

void UMusicGoldSlider::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    SlateSlider.Reset();
}

void UMusicGoldSlider::HandleSlateValueChanged(float InValue)
{
    Value = NormalizeValue(InValue);
    OnValueChanged.Broadcast(Value);
}

float UMusicGoldSlider::NormalizeValue(float InValue) const
{
    const float ClampedMax = FMath::Max(MaxValue, MinValue + UE_SMALL_NUMBER);
    float NewValue = FMath::Clamp(InValue, MinValue, ClampedMax);
    if (StepSize > 0.f)
    {
        const float Steps = FMath::RoundToFloat((NewValue - MinValue) / StepSize);
        NewValue = FMath::Clamp(MinValue + Steps * StepSize, MinValue, ClampedMax);
    }
    return NewValue;
}
