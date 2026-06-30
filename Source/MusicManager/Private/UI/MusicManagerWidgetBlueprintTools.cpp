#include "UI/MusicManagerWidgetBlueprintTools.h"

#if WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "AuditionWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/ListView.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/CommandItemWidget.h"
#include "UI/CommandPanelWidget.h"
#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "NewsFeedItemWidget.h"
#include "NewsFeedList.h"
#include "Styling/SlateBrush.h"
#include "UI/MusicGoldSlider.h"
#include "UI/MusicSegmentedMeterWidget.h"
#include "UI/RecordSongListItemWidget.h"
#include "UI/RecordWidget.h"
#include "UI/ActiveContractItemWidget.h"
#include "UI/ActiveContractsWidget.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"

namespace
{
    constexpr TCHAR TopStatusBarBlueprintPath[] = TEXT("/Game/GUI/HUD/TopStatusBarBP.TopStatusBarBP");
    constexpr TCHAR TopStatusBarSurfacePath[] = TEXT("/Game/GUI/HUD/TopStatusBarSurface.TopStatusBarSurface");
    constexpr TCHAR BrandIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_BrandRecord.TopStatusIcon_BrandRecord");
    constexpr TCHAR DateIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_DateCalendar.TopStatusIcon_DateCalendar");
    constexpr TCHAR LabelIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_LabelPerson.TopStatusIcon_LabelPerson");
    constexpr TCHAR CashIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_CashDollar.TopStatusIcon_CashDollar");
    constexpr TCHAR ReputationIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_ReputationStar.TopStatusIcon_ReputationStar");
    constexpr TCHAR PauseIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_Pause.TopStatusIcon_Pause");
    constexpr TCHAR PlayIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_Play.TopStatusIcon_Play");
    constexpr TCHAR FastForwardIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_FastForward.TopStatusIcon_FastForward");
    constexpr TCHAR MenuIconPath[] = TEXT("/Game/GUI/HUD/TopStatusIcon_Menu.TopStatusIcon_Menu");
    constexpr TCHAR CommandPanelBlueprintPath[] = TEXT("/Game/GUI/CommandPanelBP.CommandPanelBP");
    constexpr TCHAR CommandPanelBlueprintPackagePath[] = TEXT("/Game/GUI/CommandPanelBP");
    constexpr TCHAR CommandItemBlueprintPath[] = TEXT("/Game/GUI/CommandItemWidgetBP.CommandItemWidgetBP");
    constexpr TCHAR CommandItemBlueprintPackagePath[] = TEXT("/Game/GUI/CommandItemWidgetBP");
    constexpr TCHAR CommandDockBackgroundPath[] = TEXT("/Game/GUI/HUD/CommandDock/BottomCommandDock_Background.BottomCommandDock_Background");
    constexpr TCHAR CommandDockButtonPath[] = TEXT("/Game/GUI/HUD/CommandDock/BottomCommandDock_Button.BottomCommandDock_Button");
    constexpr TCHAR NewsFeedListBlueprintPath[] = TEXT("/Game/GUI/NewFeedListBP.NewFeedListBP");
    constexpr TCHAR NewsFeedListBlueprintPackagePath[] = TEXT("/Game/GUI/NewFeedListBP");
    constexpr TCHAR NewsFeedItemBlueprintPath[] = TEXT("/Game/GUI/NewsFeedItemBP.NewsFeedItemBP");
    constexpr TCHAR NewsFeedItemBlueprintPackagePath[] = TEXT("/Game/GUI/NewsFeedItemBP");
    constexpr TCHAR NewsFeedPanelSurfacePath[] = TEXT("/Game/GUI/News/NewsFeedPanelSurface.NewsFeedPanelSurface");
    constexpr TCHAR NewsFeedCardSurfacePath[] = TEXT("/Game/GUI/News/NewsFeedCardSurface.NewsFeedCardSurface");
    constexpr TCHAR NewsFeedButtonSurfacePath[] = TEXT("/Game/GUI/News/NewsFeedButtonSurface.NewsFeedButtonSurface");
    constexpr TCHAR NewsFeedMicrophoneIconPath[] = TEXT("/Game/GUI/News/NewsFeedIcon_Microphone.NewsFeedIcon_Microphone");
    constexpr TCHAR NewsFeedCalendarIconPath[] = TEXT("/Game/GUI/News/NewsFeedIcon_Calendar.NewsFeedIcon_Calendar");
    constexpr TCHAR NewsFeedListIconPath[] = TEXT("/Game/GUI/News/NewsFeedIcon_List.NewsFeedIcon_List");
    constexpr TCHAR NewsFeedChevronIconPath[] = TEXT("/Game/GUI/News/NewsFeedIcon_ChevronRight.NewsFeedIcon_ChevronRight");
    constexpr TCHAR AuditionBlueprintPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionPanelBP.ArtistAuditionPanelBP");
    constexpr TCHAR AuditionBlueprintPackagePath[] = TEXT("/Game/GUI/Audition/ArtistAuditionPanelBP");
    constexpr TCHAR AuditionPanelSurfacePath[] = TEXT("/Game/GUI/Audition/ArtistAuditionPanelSurface.ArtistAuditionPanelSurface");
    constexpr TCHAR AuditionVinylFramePath[] = TEXT("/Game/GUI/Audition/ArtistAuditionVinylFrame.ArtistAuditionVinylFrame");
    constexpr TCHAR AuditionDefaultPortraitPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionDefaultPortrait.ArtistAuditionDefaultPortrait");
    constexpr TCHAR AuditionContractSurfacePath[] = TEXT("/Game/GUI/Audition/ArtistAuditionContractSurface.ArtistAuditionContractSurface");
    constexpr TCHAR AuditionSignButtonSurfacePath[] = TEXT("/Game/GUI/Audition/ArtistAuditionSignButtonSurface.ArtistAuditionSignButtonSurface");
    constexpr TCHAR AuditionPassButtonSurfacePath[] = TEXT("/Game/GUI/Audition/ArtistAuditionPassButtonSurface.ArtistAuditionPassButtonSurface");
    constexpr TCHAR AuditionRecordIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Record.ArtistAuditionIcon_Record");
    constexpr TCHAR AuditionLocationIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Location.ArtistAuditionIcon_Location");
    constexpr TCHAR AuditionPerformanceIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Performance.ArtistAuditionIcon_Performance");
    constexpr TCHAR AuditionStageIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Stage.ArtistAuditionIcon_Stage");
    constexpr TCHAR AuditionAudienceIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Audience.ArtistAuditionIcon_Audience");
    constexpr TCHAR AuditionVocalIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Vocal.ArtistAuditionIcon_Vocal");
    constexpr TCHAR AuditionSongwritingIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Songwriting.ArtistAuditionIcon_Songwriting");
    constexpr TCHAR AuditionContractIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Contract.ArtistAuditionIcon_Contract");
    constexpr TCHAR StudioClockIconPath[] = TEXT("/Game/GUI/StudioRecording/StudioRecordingIcon_Clock.StudioRecordingIcon_Clock");
    constexpr TCHAR StudioWarningIconPath[] = TEXT("/Game/GUI/StudioRecording/StudioRecordingIcon_Warning.StudioRecordingIcon_Warning");
    constexpr TCHAR StudioFilterIconPath[] = TEXT("/Game/GUI/StudioRecording/StudioRecordingIcon_Filter.StudioRecordingIcon_Filter");
    constexpr TCHAR StudioSearchIconPath[] = TEXT("/Game/GUI/StudioRecording/StudioRecordingIcon_Search.StudioRecordingIcon_Search");
    constexpr TCHAR AuditionGiftIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_Gift.ArtistAuditionIcon_Gift");
    constexpr TCHAR AuditionPlusPersonIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_PlusPerson.ArtistAuditionIcon_PlusPerson");
    constexpr TCHAR AuditionPassArrowIconPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionIcon_PassArrow.ArtistAuditionIcon_PassArrow");
    constexpr TCHAR RecordingBlueprintPath[] = TEXT("/Game/GUI/RecordingGUIBP.RecordingGUIBP");
    constexpr TCHAR RecordingBlueprintPackagePath[] = TEXT("/Game/GUI/RecordingGUIBP");
    constexpr TCHAR RecordSongListBlueprintPath[] = TEXT("/Game/GUI/RecordSongListBP.RecordSongListBP");
    constexpr TCHAR RecordSongListBlueprintPackagePath[] = TEXT("/Game/GUI/RecordSongListBP");
    constexpr TCHAR RecordingRecordListBlueprintPath[] = TEXT("/Game/GUI/RecordingRecordListBP.RecordingRecordListBP");
    constexpr TCHAR RecordingRecordListBlueprintPackagePath[] = TEXT("/Game/GUI/RecordingRecordListBP");
    constexpr TCHAR ActiveContractsBlueprintPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsBP.ActiveContractsBP");
    constexpr TCHAR ActiveContractsBlueprintPackagePath[] = TEXT("/Game/GUI/Contracts/ActiveContractsBP");
    constexpr TCHAR ActiveContractItemBlueprintPath[] = TEXT("/Game/GUI/Contracts/ActiveContractItemBP.ActiveContractItemBP");
    constexpr TCHAR ActiveContractItemBlueprintPackagePath[] = TEXT("/Game/GUI/Contracts/ActiveContractItemBP");
    constexpr TCHAR ContractsContractIconPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsIcon_Contract.ActiveContractsIcon_Contract");
    constexpr TCHAR ContractsArtistIconPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsIcon_Artist.ActiveContractsIcon_Artist");
    constexpr TCHAR ContractsCalendarIconPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsIcon_Calendar.ActiveContractsIcon_Calendar");
    constexpr TCHAR ContractsRoyaltyIconPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsIcon_Royalty.ActiveContractsIcon_Royalty");
    constexpr TCHAR ContractsBonusIconPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsIcon_Bonus.ActiveContractsIcon_Bonus");
    constexpr TCHAR ContractsRevenueIconPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsIcon_Revenue.ActiveContractsIcon_Revenue");
    constexpr TCHAR ContractsRecordIconPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsIcon_Record.ActiveContractsIcon_Record");
    constexpr TCHAR ContractsCloseIconPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsIcon_Close.ActiveContractsIcon_Close");

    const FLinearColor Transparent(0.f, 0.f, 0.f, 0.f);
    const FLinearColor PillBlack(0.025f, 0.026f, 0.022f, 0.96f);
    const FLinearColor BorderGold(0.88f, 0.56f, 0.13f, 0.92f);
    const FLinearColor TextIvory(0.91f, 0.882f, 0.824f, 1.f);
    const FLinearColor MutedText(0.66f, 0.616f, 0.55f, 1.f);
    const FLinearColor Gold(1.f, 0.827f, 0.353f, 1.f);

    template<typename T>
    T* ConstructWidget(UWidgetTree* Tree, const TCHAR* Name, bool bVariable = true)
    {
        T* Widget = Tree->ConstructWidget<T>(T::StaticClass(), FName(Name));
        Widget->bIsVariable = bVariable;
        return Widget;
    }

    UCanvasPanelSlot* AddToCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FMargin& Offsets)
    {
        UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
        Slot->SetAnchors(Anchors);
        Slot->SetOffsets(Offsets);
        Slot->SetAlignment(FVector2D(0.f, 0.f));
        return Slot;
    }

    UHorizontalBoxSlot* AddToRow(UHorizontalBox* Row, UWidget* Widget, const FMargin& Padding, ESlateSizeRule::Type SizeRule = ESlateSizeRule::Automatic, float Value = 1.f)
    {
        UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Widget);
        Slot->SetPadding(Padding);
        Slot->SetVerticalAlignment(VAlign_Center);
        FSlateChildSize ChildSize;
        ChildSize.SizeRule = SizeRule;
        ChildSize.Value = Value;
        Slot->SetSize(ChildSize);
        return Slot;
    }

    void ConfigureImage(UImage* Image, const TCHAR* TexturePath, const FVector2D& ImageSize)
    {
        if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath))
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(Texture);
            Brush.ImageSize = ImageSize;
            Image->SetBrush(Brush);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Top status bar rebuild could not load texture %s."), TexturePath);
        }
    }

    UImage* AddIconToCanvas(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* WidgetName, const TCHAR* TexturePath, const FMargin& Offsets, float Size)
    {
        UImage* Icon = ConstructWidget<UImage>(Tree, WidgetName);
        ConfigureImage(Icon, TexturePath, FVector2D(Size, Size));
        AddToCanvas(Canvas, Icon, FAnchors(0.f, 0.f), Offsets);
        return Icon;
    }

    void ConfigureText(UTextBlock* TextBlock, const FString& Text, int32 Size, const FLinearColor& Color)
    {
        TextBlock->SetText(FText::FromString(Text));
        TextBlock->SetColorAndOpacity(FSlateColor(Color));
        TextBlock->SetJustification(ETextJustify::Left);

        FSlateFontInfo Font = TextBlock->GetFont();
        Font.Size = Size;
        TextBlock->SetFont(Font);
    }

    void ConfigureWrappingText(UTextBlock* TextBlock, const FString& Text, int32 Size, const FLinearColor& Color)
    {
        ConfigureText(TextBlock, Text, Size, Color);
        TextBlock->SetAutoWrapText(true);
    }

    UImage* MakeIcon(UWidgetTree* Tree, const TCHAR* WidgetName, const TCHAR* TexturePath, float Size)
    {
        UImage* Icon = ConstructWidget<UImage>(Tree, WidgetName);
        ConfigureImage(Icon, TexturePath, FVector2D(Size, Size));
        Icon->SetColorAndOpacity(FLinearColor::White);
        return Icon;
    }

    UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* WidgetName, const FString& Value, int32 Size, const FLinearColor& Color)
    {
        UTextBlock* TextBlock = ConstructWidget<UTextBlock>(Tree, WidgetName);
        ConfigureText(TextBlock, Value, Size, Color);
        return TextBlock;
    }

    USizeBox* MakePill(UWidgetTree* Tree, const TCHAR* PillName, float Width, UImage* Icon, UTextBlock* Text)
    {
        USizeBox* Size = ConstructWidget<USizeBox>(Tree, *FString::Printf(TEXT("%sSize"), PillName), false);
        Size->SetWidthOverride(Width);
        Size->SetHeightOverride(44.f);

        UBorder* Border = ConstructWidget<UBorder>(Tree, *FString::Printf(TEXT("%sBorder"), PillName), false);
        Border->SetBrushColor(PillBlack);
        Border->SetPadding(FMargin(14.f, 0.f, 14.f, 0.f));
        Size->AddChild(Border);

        UHorizontalBox* Content = ConstructWidget<UHorizontalBox>(Tree, *FString::Printf(TEXT("%sContent"), PillName), false);
        Border->SetContent(Content);

        AddToRow(Content, Icon, FMargin(0.f, 0.f, 10.f, 0.f));
        AddToRow(Content, Text, FMargin(0.f));
        return Size;
    }

    void ConfigureButton(UButton* Button)
    {
        Button->SetBackgroundColor(FLinearColor(0.07f, 0.052f, 0.024f, 1.f));
    }

    UImage* AddButtonIcon(UWidgetTree* Tree, UButton* Button, const TCHAR* Name, const TCHAR* TexturePath)
    {
        USizeBox* Size = ConstructWidget<USizeBox>(Tree, *FString::Printf(TEXT("%s_Size"), Name), false);
        Size->SetWidthOverride(48.f);
        Size->SetHeightOverride(40.f);

        UImage* Icon = ConstructWidget<UImage>(Tree, *FString::Printf(TEXT("%sIconImage"), Name));
        ConfigureImage(Icon, TexturePath, FVector2D(34.f, 34.f));

        Size->AddChild(Icon);
        Button->AddChild(Size);
        return Icon;
    }

    void RegisterWidgetVariableGuids(UWidgetBlueprint* WidgetBlueprint);

    void SaveAndCompileWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint, const TCHAR* Context)
    {
        RegisterWidgetVariableGuids(WidgetBlueprint);
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
        UPackage* Package = WidgetBlueprint->GetOutermost();
        Package->MarkPackageDirty();
        const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        const bool bSaved = UPackage::SavePackage(Package, WidgetBlueprint, *PackageFilename, FSavePackageArgs());
        UE_LOG(LogTemp, Log, TEXT("%s: Saved=%s Package=%s."), Context, bSaved ? TEXT("true") : TEXT("false"), *Package->GetName());
    }

    void RegisterWidgetVariableGuids(UWidgetBlueprint* WidgetBlueprint)
    {
        if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
        {
            return;
        }

        WidgetBlueprint->WidgetVariableNameToGuidMap.Empty();
        WidgetBlueprint->WidgetTree->ForEachWidget([WidgetBlueprint](UWidget* Widget)
        {
            if (Widget && !Widget->GetFName().IsNone())
            {
                WidgetBlueprint->WidgetVariableNameToGuidMap.FindOrAdd(Widget->GetFName()) = FGuid::NewGuid();
            }
        });

    }

    void ClearWidgetTree(UWidgetTree* Tree)
    {
        if (!Tree)
        {
            return;
        }

        TArray<UWidget*> ExistingWidgets;
        Tree->GetAllWidgets(ExistingWidgets);
        for (UWidget* Widget : ExistingWidgets)
        {
            if (Widget)
            {
                Tree->RemoveWidget(Widget);
            }
        }

        Tree->NamedSlotBindings.Empty();
        Tree->RootWidget = nullptr;
    }

    UWidgetBlueprint* LoadOrCreateWidgetBlueprint(const TCHAR* ObjectPath, const TCHAR* PackagePath, const TCHAR* AssetName, UClass* ParentClass)
    {
        UWidgetBlueprint* WidgetBlueprint = LoadObject<UWidgetBlueprint>(nullptr, ObjectPath);
        if (WidgetBlueprint)
        {
            return WidgetBlueprint;
        }

        UPackage* Package = CreatePackage(PackagePath);
        WidgetBlueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
            ParentClass,
            Package,
            AssetName,
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass()));

        if (WidgetBlueprint)
        {
            FAssetRegistryModule::AssetCreated(WidgetBlueprint);
        }

        return WidgetBlueprint;
    }

    bool SetBlueprintSubclassDefault(UWidgetBlueprint* WidgetBlueprint, const FName PropertyName, UClass* ClassValue)
    {
        if (!WidgetBlueprint || !WidgetBlueprint->GeneratedClass || !ClassValue)
        {
            return false;
        }

        UObject* DefaultObject = WidgetBlueprint->GeneratedClass->GetDefaultObject();
        if (!DefaultObject)
        {
            return false;
        }

        if (FClassProperty* ClassProperty = FindFProperty<FClassProperty>(DefaultObject->GetClass(), PropertyName))
        {
            ClassProperty->SetPropertyValue_InContainer(DefaultObject, ClassValue);
            DefaultObject->Modify();
            return true;
        }

        return false;
    }

    bool SetBlueprintBoolDefault(UWidgetBlueprint* WidgetBlueprint, const FName PropertyName, const bool bValue)
    {
        if (!WidgetBlueprint || !WidgetBlueprint->GeneratedClass)
        {
            return false;
        }

        UObject* DefaultObject = WidgetBlueprint->GeneratedClass->GetDefaultObject();
        if (!DefaultObject)
        {
            return false;
        }

        if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(DefaultObject->GetClass(), PropertyName))
        {
            BoolProperty->SetPropertyValue_InContainer(DefaultObject, bValue);
            DefaultObject->Modify();
            return true;
        }

        return false;
    }

    bool SetBlueprintObjectDefault(UWidgetBlueprint* WidgetBlueprint, const FName PropertyName, UObject* ObjectValue)
    {
        if (!WidgetBlueprint || !WidgetBlueprint->GeneratedClass)
        {
            return false;
        }

        UObject* DefaultObject = WidgetBlueprint->GeneratedClass->GetDefaultObject();
        if (!DefaultObject)
        {
            return false;
        }

        if (FObjectProperty* ObjectProperty = FindFProperty<FObjectProperty>(DefaultObject->GetClass(), PropertyName))
        {
            ObjectProperty->SetObjectPropertyValue_InContainer(DefaultObject, ObjectValue);
            DefaultObject->Modify();
            return true;
        }

        return false;
    }

    bool SetListViewEntryWidgetClass(UListView* ListView, UClass* EntryWidgetClass)
    {
        if (!ListView || !EntryWidgetClass)
        {
            return false;
        }

        if (FClassProperty* ClassProperty = FindFProperty<FClassProperty>(ListView->GetClass(), TEXT("EntryWidgetClass")))
        {
            ClassProperty->SetPropertyValue_InContainer(ListView, EntryWidgetClass);
            ListView->Modify();
            return true;
        }

        UE_LOG(LogTemp, Warning, TEXT("SetListViewEntryWidgetClass: EntryWidgetClass property was not found on %s."), *GetNameSafe(ListView));
        return false;
    }

    void AddCanvasImage(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Name, const TCHAR* TexturePath, const FMargin& Offsets, const FVector2D& ImageSize, bool bVariable = true)
    {
        UImage* Image = ConstructWidget<UImage>(Tree, Name, bVariable);
        ConfigureImage(Image, TexturePath, ImageSize);
        AddToCanvas(Canvas, Image, FAnchors(0.f, 0.f), Offsets);
    }

    UTextBlock* AddCanvasText(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Name, const FString& Value, int32 Size, const FLinearColor& Color, const FMargin& Offsets, bool bVariable = true)
    {
        UTextBlock* Text = ConstructWidget<UTextBlock>(Tree, Name, bVariable);
        ConfigureText(Text, Value, Size, Color);
        AddToCanvas(Canvas, Text, FAnchors(0.f, 0.f), Offsets);
        return Text;
    }

    UMusicSegmentedMeterWidget* AddMeter(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Name, const FMargin& Offsets)
    {
        UMusicSegmentedMeterWidget* Meter = ConstructWidget<UMusicSegmentedMeterWidget>(Tree, Name, false);
        Meter->SetPercent(0.72f);
        AddToCanvas(Canvas, Meter, FAnchors(0.f, 0.f), Offsets);
        return Meter;
    }

    void AddStatRow(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Label, const TCHAR* IconPath, const TCHAR* MeterName, const TCHAR* ValueTextName, float Y)
    {
        UImage* Icon = ConstructWidget<UImage>(Tree, *FString::Printf(TEXT("%sIcon"), MeterName));
        ConfigureImage(Icon, IconPath, FVector2D(26.f, 26.f));
        AddToCanvas(Canvas, Icon, FAnchors(0.f, 0.f), FMargin(50.f, Y - 2.f, 26.f, 26.f));

        AddCanvasText(Tree, Canvas, *FString::Printf(TEXT("%sLabel"), MeterName), Label, 13, MutedText, FMargin(94.f, Y, 160.f, 22.f), false);
        AddMeter(Tree, Canvas, MeterName, FMargin(246.f, Y + 1.f, 182.f, 20.f));
        AddCanvasText(Tree, Canvas, ValueTextName, TEXT("0"), 15, TextIvory, FMargin(448.f, Y - 3.f, 42.f, 26.f));
    }

    void AddDealRow(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Label, const TCHAR* IconPath, const TCHAR* SliderName, const TCHAR* ValueName, float Y, const TCHAR* IconWidgetName = nullptr)
    {
        const FString GeneratedIconName = FString::Printf(TEXT("%sIcon"), SliderName);
        UImage* Icon = ConstructWidget<UImage>(Tree, IconWidgetName ? IconWidgetName : *GeneratedIconName, false);
        ConfigureImage(Icon, IconPath, FVector2D(24.f, 24.f));
        AddToCanvas(Canvas, Icon, FAnchors(0.f, 0.f), FMargin(56.f, Y - 1.f, 24.f, 24.f));

        AddCanvasText(Tree, Canvas, *FString::Printf(TEXT("%sLabel"), SliderName), Label, 13, MutedText, FMargin(94.f, Y, 110.f, 22.f), false);

        UMusicGoldSlider* Slider = ConstructWidget<UMusicGoldSlider>(Tree, SliderName);
        Slider->SetValue(0.5f);
        AddToCanvas(Canvas, Slider, FAnchors(0.f, 0.f), FMargin(194.f, Y + 2.f, 174.f, 26.f));

        UBorder* ValueBorder = ConstructWidget<UBorder>(Tree, *FString::Printf(TEXT("%sValueBorder"), SliderName), false);
        ValueBorder->SetBrushColor(FLinearColor(0.025f, 0.026f, 0.022f, 0.95f));
        ValueBorder->SetPadding(FMargin(8.f, 0.f, 8.f, 0.f));
        AddToCanvas(Canvas, ValueBorder, FAnchors(0.f, 0.f), FMargin(390.f, Y - 4.f, 86.f, 34.f));

        UTextBlock* ValueText = ConstructWidget<UTextBlock>(Tree, ValueName);
        ConfigureText(ValueText, TEXT("0"), 14, TextIvory);
        ValueText->SetJustification(ETextJustify::Center);
        ValueBorder->SetContent(ValueText);
    }

    UButton* AddAuditionButton(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* ButtonName, const TCHAR* BackgroundName, const TCHAR* SurfacePath, const TCHAR* IconName, const TCHAR* IconPath, const TCHAR* Label, const FMargin& Offsets, const FVector2D& SurfaceSize)
    {
        UButton* Button = ConstructWidget<UButton>(Tree, ButtonName);
        Button->SetBackgroundColor(Transparent);
        AddToCanvas(Canvas, Button, FAnchors(0.f, 0.f), Offsets);

        UCanvasPanel* ButtonCanvas = ConstructWidget<UCanvasPanel>(Tree, *FString::Printf(TEXT("%sCanvas"), ButtonName), false);
        Button->AddChild(ButtonCanvas);
        AddCanvasImage(Tree, ButtonCanvas, BackgroundName, SurfacePath, FMargin(0.f, 0.f, SurfaceSize.X, SurfaceSize.Y), SurfaceSize);

        UImage* Icon = ConstructWidget<UImage>(Tree, IconName, false);
        ConfigureImage(Icon, IconPath, FVector2D(32.f, 32.f));
        AddToCanvas(ButtonCanvas, Icon, FAnchors(0.f, 0.f), FMargin(22.f, 16.f, 32.f, 32.f));

        UTextBlock* ButtonText = ConstructWidget<UTextBlock>(Tree, *FString::Printf(TEXT("%sLabel"), ButtonName), false);
        ConfigureText(ButtonText, Label, 15, TextIvory);
        ButtonText->SetJustification(ETextJustify::Center);
        AddToCanvas(ButtonCanvas, ButtonText, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(60.f, 17.f, 18.f, 20.f));
        return Button;
    }

    bool RebuildNewsFeedItemBlueprint()
    {
        UWidgetBlueprint* WidgetBlueprint = LoadOrCreateWidgetBlueprint(
            NewsFeedItemBlueprintPath,
            NewsFeedItemBlueprintPackagePath,
            TEXT("NewsFeedItemBP"),
            UNewsFeedItemWidget::StaticClass());
        if (!WidgetBlueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildNewsFeedItemBlueprint: Could not load %s."), NewsFeedItemBlueprintPath);
            return false;
        }

        if (WidgetBlueprint->ParentClass != UNewsFeedItemWidget::StaticClass())
        {
            WidgetBlueprint->ParentClass = UNewsFeedItemWidget::StaticClass();
        }

        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildNewsFeedItemBlueprint: WidgetTree is missing."));
            return false;
        }

        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        USizeBox* RootSizeBox = ConstructWidget<USizeBox>(Tree, TEXT("RootSizeBox"));
        RootSizeBox->SetWidthOverride(448.f);
        RootSizeBox->SetHeightOverride(148.f);
        Tree->RootWidget = RootSizeBox;

        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"));
        RootSizeBox->AddChild(RootCanvas);

        AddCanvasImage(Tree, RootCanvas, TEXT("CardBackgroundImage"), NewsFeedCardSurfacePath, FMargin(0.f, 0.f, 448.f, 148.f), FVector2D(448.f, 148.f));

        UImage* NewsTypeIcon = ConstructWidget<UImage>(Tree, TEXT("NewsTypeIcon"));
        ConfigureImage(NewsTypeIcon, NewsFeedMicrophoneIconPath, FVector2D(58.f, 58.f));
        AddToCanvas(RootCanvas, NewsTypeIcon, FAnchors(0.f, 0.f), FMargin(24.f, 44.f, 58.f, 58.f));

        UImage* AccentDivider = ConstructWidget<UImage>(Tree, TEXT("AccentDivider"));
        AccentDivider->SetColorAndOpacity(Gold);
        AddToCanvas(RootCanvas, AccentDivider, FAnchors(0.f, 0.f), FMargin(100.f, 26.f, 4.f, 96.f));

        UTextBlock* HeadlineText = ConstructWidget<UTextBlock>(Tree, TEXT("HeadlineText"));
        ConfigureWrappingText(HeadlineText, TEXT("Headline"), 19, TextIvory);
        AddToCanvas(RootCanvas, HeadlineText, FAnchors(0.f, 0.f), FMargin(122.f, 22.f, 286.f, 56.f));

        UTextBlock* SourceText = ConstructWidget<UTextBlock>(Tree, TEXT("SourceText"));
        ConfigureText(SourceText, TEXT("Source"), 14, MutedText);
        AddToCanvas(RootCanvas, SourceText, FAnchors(0.f, 0.f), FMargin(122.f, 82.f, 286.f, 24.f));

        UImage* DateIcon = ConstructWidget<UImage>(Tree, TEXT("DateIcon"));
        ConfigureImage(DateIcon, NewsFeedCalendarIconPath, FVector2D(18.f, 18.f));
        AddToCanvas(RootCanvas, DateIcon, FAnchors(0.f, 0.f), FMargin(122.f, 112.f, 18.f, 18.f));

        UTextBlock* DateText = ConstructWidget<UTextBlock>(Tree, TEXT("DateText"));
        ConfigureText(DateText, TEXT("Jan 12, 1955"), 13, MutedText);
        AddToCanvas(RootCanvas, DateText, FAnchors(0.f, 0.f), FMargin(146.f, 108.f, 190.f, 24.f));

        RegisterWidgetVariableGuids(WidgetBlueprint);
        SaveAndCompileWidgetBlueprint(WidgetBlueprint, TEXT("RebuildNewsFeedItemBlueprint"));
        return true;
    }

    bool RebuildNewsFeedListBlueprint()
    {
        UWidgetBlueprint* WidgetBlueprint = LoadOrCreateWidgetBlueprint(
            NewsFeedListBlueprintPath,
            NewsFeedListBlueprintPackagePath,
            TEXT("NewFeedListBP"),
            UNewsFeedList::StaticClass());
        UWidgetBlueprint* ItemBlueprint = LoadObject<UWidgetBlueprint>(nullptr, NewsFeedItemBlueprintPath);
        if (!WidgetBlueprint || !ItemBlueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildNewsFeedListBlueprint: Could not load list or item Blueprint."));
            return false;
        }

        if (WidgetBlueprint->ParentClass != UNewsFeedList::StaticClass())
        {
            WidgetBlueprint->ParentClass = UNewsFeedList::StaticClass();
        }

        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildNewsFeedListBlueprint: WidgetTree is missing."));
            return false;
        }

        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"));
        Tree->RootWidget = RootCanvas;

        AddCanvasImage(Tree, RootCanvas, TEXT("PanelBackgroundImage"), NewsFeedPanelSurfacePath, FMargin(0.f, 0.f, 520.f, 1280.f), FVector2D(520.f, 1280.f));

        UTextBlock* HeaderText = ConstructWidget<UTextBlock>(Tree, TEXT("HeaderText"));
        ConfigureText(HeaderText, TEXT("NEWS FEED"), 18, TextIvory);
        AddToCanvas(RootCanvas, HeaderText, FAnchors(0.f, 0.f), FMargin(32.f, 28.f, 250.f, 30.f));

        UScrollBox* FeedScrollBox = ConstructWidget<UScrollBox>(Tree, TEXT("FeedScrollBox"));
        FeedScrollBox->SetAnimateWheelScrolling(true);
        FeedScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
        AddToCanvas(RootCanvas, FeedScrollBox, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(36.f, 106.f, 36.f, 132.f));

        UVerticalBox* FeedContainer = ConstructWidget<UVerticalBox>(Tree, TEXT("FeedContainer"));
        if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(FeedScrollBox->AddChild(FeedContainer)))
        {
            ScrollSlot->SetHorizontalAlignment(HAlign_Fill);
        }

        UButton* ViewAllNewsButton = ConstructWidget<UButton>(Tree, TEXT("ViewAllNewsButton"));
        ViewAllNewsButton->SetBackgroundColor(Transparent);
        AddToCanvas(RootCanvas, ViewAllNewsButton, FAnchors(0.f, 1.f, 1.f, 1.f), FMargin(26.f, -100.f, 26.f, 72.f));

        UCanvasPanel* ViewAllButtonCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("ViewAllNewsButtonCanvas"), false);
        ViewAllNewsButton->AddChild(ViewAllButtonCanvas);

        AddCanvasImage(Tree, ViewAllButtonCanvas, TEXT("ViewAllNewsButtonBackground"), NewsFeedButtonSurfacePath, FMargin(0.f, 0.f, 468.f, 72.f), FVector2D(438.f, 76.f));

        UImage* ViewAllNewsIcon = ConstructWidget<UImage>(Tree, TEXT("ViewAllNewsIcon"));
        ConfigureImage(ViewAllNewsIcon, NewsFeedListIconPath, FVector2D(34.f, 34.f));
        AddToCanvas(ViewAllButtonCanvas, ViewAllNewsIcon, FAnchors(0.f, 0.f), FMargin(30.f, 18.f, 34.f, 34.f));

        UTextBlock* ViewAllNewsText = ConstructWidget<UTextBlock>(Tree, TEXT("ViewAllNewsText"));
        ConfigureText(ViewAllNewsText, TEXT("VIEW ALL NEWS"), 15, TextIvory);
        AddToCanvas(ViewAllButtonCanvas, ViewAllNewsText, FAnchors(0.f, 0.f), FMargin(98.f, 21.f, 230.f, 28.f));

        UImage* ViewAllNewsChevron = ConstructWidget<UImage>(Tree, TEXT("ViewAllNewsChevron"));
        ConfigureImage(ViewAllNewsChevron, NewsFeedChevronIconPath, FVector2D(28.f, 28.f));
        AddToCanvas(ViewAllButtonCanvas, ViewAllNewsChevron, FAnchors(1.f, 0.f), FMargin(-54.f, 21.f, 28.f, 28.f));

        UCanvasPanel* HoverCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("HoverCanvas"));
        HoverCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        AddToCanvas(RootCanvas, HoverCanvas, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));

        RegisterWidgetVariableGuids(WidgetBlueprint);
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
        SetBlueprintSubclassDefault(WidgetBlueprint, TEXT("NewsFeedItemWidgetClass"), ItemBlueprint->GeneratedClass);
        SetBlueprintBoolDefault(WidgetBlueprint, TEXT("bClearFeedOnConstruct"), true);

        UPackage* Package = WidgetBlueprint->GetOutermost();
        Package->MarkPackageDirty();
        const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        const bool bSaved = UPackage::SavePackage(Package, WidgetBlueprint, *PackageFilename, FSavePackageArgs());
        UE_LOG(LogTemp, Log, TEXT("RebuildNewsFeedListBlueprint: Rebuilt %s Saved=%s."), NewsFeedListBlueprintPath, bSaved ? TEXT("true") : TEXT("false"));
        return bSaved;
    }

    bool RebuildAuditionPanelBlueprint()
    {
        UWidgetBlueprint* WidgetBlueprint = LoadObject<UWidgetBlueprint>(nullptr, AuditionBlueprintPath);
        if (!WidgetBlueprint)
        {
            UPackage* Package = CreatePackage(AuditionBlueprintPackagePath);
            WidgetBlueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
                UAuditionWidget::StaticClass(),
                Package,
                TEXT("ArtistAuditionPanelBP"),
                BPTYPE_Normal,
                UWidgetBlueprint::StaticClass(),
                UWidgetBlueprintGeneratedClass::StaticClass()));

            if (!WidgetBlueprint)
            {
                UE_LOG(LogTemp, Error, TEXT("RebuildAuditionPanelBlueprint: Could not load or create %s."), AuditionBlueprintPath);
                return false;
            }

            FAssetRegistryModule::AssetCreated(WidgetBlueprint);
        }

        if (WidgetBlueprint->ParentClass != UAuditionWidget::StaticClass())
        {
            WidgetBlueprint->ParentClass = UAuditionWidget::StaticClass();
        }

        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildAuditionPanelBlueprint: WidgetTree is missing."));
            return false;
        }

        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        USizeBox* RootSizeBox = ConstructWidget<USizeBox>(Tree, TEXT("RootSizeBox"), false);
        RootSizeBox->SetWidthOverride(520.f);
        RootSizeBox->SetHeightOverride(720.f);
        RootSizeBox->SetClipping(EWidgetClipping::ClipToBounds);
        Tree->RootWidget = RootSizeBox;

        UCanvasPanel* ShellCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"));
        ShellCanvas->SetClipping(EWidgetClipping::ClipToBounds);
        RootSizeBox->AddChild(ShellCanvas);

        AddCanvasImage(Tree, ShellCanvas, TEXT("PanelBackgroundImage"), AuditionPanelSurfacePath, FMargin(0.f, 0.f, 520.f, 1280.f), FVector2D(520.f, 1280.f));

        UImage* HeaderIcon = ConstructWidget<UImage>(Tree, TEXT("HeaderRecordIcon"));
        ConfigureImage(HeaderIcon, AuditionRecordIconPath, FVector2D(50.f, 50.f));
        AddToCanvas(ShellCanvas, HeaderIcon, FAnchors(0.f, 0.f), FMargin(42.f, 18.f, 50.f, 50.f));
        AddCanvasText(Tree, ShellCanvas, TEXT("AuditionHeaderText"), TEXT("ARTIST AUDITION"), 20, Gold, FMargin(118.f, 25.f, 270.f, 34.f), false);

        UScrollBox* AuditionScrollBox = ConstructWidget<UScrollBox>(Tree, TEXT("AuditionScrollBox"));
        AuditionScrollBox->SetAnimateWheelScrolling(true);
        AuditionScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
        AuditionScrollBox->SetClipping(EWidgetClipping::ClipToBounds);
        AddToCanvas(ShellCanvas, AuditionScrollBox, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f, 126.f, 0.f, 18.f));

        USizeBox* ContentSizeBox = ConstructWidget<USizeBox>(Tree, TEXT("AuditionContentSizeBox"), false);
        ContentSizeBox->SetWidthOverride(520.f);
        ContentSizeBox->SetHeightOverride(1220.f);
        AuditionScrollBox->AddChild(ContentSizeBox);

        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("AuditionContentCanvas"), false);
        ContentSizeBox->AddChild(RootCanvas);

        AddCanvasText(Tree, RootCanvas, TEXT("TextArtistName"), TEXT("Artist"), 25, Gold, FMargin(48.f, 20.f, 350.f, 46.f));
        AddCanvasText(Tree, RootCanvas, TEXT("TextGenre"), TEXT("Genre"), 15, TextIvory, FMargin(50.f, 68.f, 260.f, 28.f));

        UImage* ArtistPortraitImage = ConstructWidget<UImage>(Tree, TEXT("ArtistPortraitImage"));
        ConfigureImage(ArtistPortraitImage, AuditionDefaultPortraitPath, FVector2D(132.f, 132.f));
        FSlateBrush PortraitBrush = ArtistPortraitImage->GetBrush();
        PortraitBrush.SetUVRegion(FBox2f(FVector2f(0.16f, 0.03f), FVector2f(0.84f, 0.71f)));
        ArtistPortraitImage->SetBrush(PortraitBrush);
        AddToCanvas(RootCanvas, ArtistPortraitImage, FAnchors(0.f, 0.f), FMargin(194.f, 224.f, 132.f, 132.f));

        UImage* VinylFrameImage = ConstructWidget<UImage>(Tree, TEXT("VinylFrameImage"));
        ConfigureImage(VinylFrameImage, AuditionVinylFramePath, FVector2D(356.f, 356.f));
        AddToCanvas(RootCanvas, VinylFrameImage, FAnchors(0.f, 0.f), FMargin(82.f, 112.f, 356.f, 356.f));

        UImage* LocationIcon = ConstructWidget<UImage>(Tree, TEXT("LocationIcon"));
        ConfigureImage(LocationIcon, AuditionLocationIconPath, FVector2D(38.f, 38.f));
        AddToCanvas(RootCanvas, LocationIcon, FAnchors(0.f, 0.f), FMargin(50.f, 494.f, 38.f, 38.f));
        AddCanvasText(Tree, RootCanvas, TEXT("VenueLabelText"), TEXT("LOCAL VENUE"), 12, MutedText, FMargin(110.f, 488.f, 180.f, 22.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("TextVenue"), TEXT("Venue"), 15, TextIvory, FMargin(110.f, 512.f, 220.f, 26.f));
        AddCanvasText(Tree, RootCanvas, TEXT("TextCity"), TEXT("City"), 12, MutedText, FMargin(110.f, 538.f, 220.f, 22.f));

        AddStatRow(Tree, RootCanvas, TEXT("PERFORMANCE"), AuditionPerformanceIconPath, TEXT("PerformanceMeter"), TEXT("TextPerformanceScore"), 586.f);
        AddStatRow(Tree, RootCanvas, TEXT("STAGE PRESENCE"), AuditionStageIconPath, TEXT("StagePresenceMeter"), TEXT("TextStagePresence"), 630.f);
        AddStatRow(Tree, RootCanvas, TEXT("AUDIENCE"), AuditionAudienceIconPath, TEXT("AudienceEngagementMeter"), TEXT("TextAudienceEngagement"), 674.f);
        AddStatRow(Tree, RootCanvas, TEXT("VOCAL"), AuditionVocalIconPath, TEXT("VocalQualityMeter"), TEXT("TextVocalQuality"), 718.f);
        AddStatRow(Tree, RootCanvas, TEXT("SONGWRITING"), AuditionSongwritingIconPath, TEXT("SongwritingQualityMeter"), TEXT("TextSongwritingQuality"), 762.f);

        AddCanvasImage(Tree, RootCanvas, TEXT("ContractSurfaceImage"), AuditionContractSurfacePath, FMargin(26.f, 822.f, 468.f, 270.f), FVector2D(468.f, 270.f), false);
        UImage* ContractIcon = ConstructWidget<UImage>(Tree, TEXT("ContractIcon"));
        ConfigureImage(ContractIcon, AuditionContractIconPath, FVector2D(28.f, 28.f));
        AddToCanvas(RootCanvas, ContractIcon, FAnchors(0.f, 0.f), FMargin(52.f, 842.f, 28.f, 28.f));
        AddCanvasText(Tree, RootCanvas, TEXT("ContractHeaderText"), TEXT("CONTRACT OFFER"), 16, Gold, FMargin(94.f, 842.f, 220.f, 28.f), false);

        AddDealRow(Tree, RootCanvas, TEXT("BONUS"), AuditionGiftIconPath, TEXT("SliderSignUpBonus"), TEXT("TextSignUpBonusValue"), 894.f);
        UImage* LegacyGiftIcon = ConstructWidget<UImage>(Tree, TEXT("GiftIcon"), false);
        LegacyGiftIcon->SetVisibility(ESlateVisibility::Collapsed);
        AddToCanvas(RootCanvas, LegacyGiftIcon, FAnchors(0.f, 0.f), FMargin(0.f, 0.f, 1.f, 1.f));
        AddDealRow(Tree, RootCanvas, TEXT("RECORDS"), AuditionRecordIconPath, TEXT("SliderNumOfRecords"), TEXT("TextNumOfRecordsValue"), 936.f);
        AddDealRow(Tree, RootCanvas, TEXT("ROYALTY"), AuditionContractIconPath, TEXT("SliderRoyaltyRate"), TEXT("TextRoyaltyRateValue"), 978.f);
        AddDealRow(Tree, RootCanvas, TEXT("TERM"), AuditionContractIconPath, TEXT("SliderContractYears"), TEXT("TextContractYearsValue"), 1020.f);

        AddAuditionButton(Tree, RootCanvas, TEXT("ButtonSignArtist"), TEXT("SignButtonBackground"), AuditionSignButtonSurfacePath, TEXT("SignButtonIcon"), AuditionPlusPersonIconPath, TEXT("SIGN ARTIST"), FMargin(28.f, 1122.f, 222.f, 68.f), FVector2D(222.f, 68.f));
        AddAuditionButton(Tree, RootCanvas, TEXT("ButtonPass"), TEXT("PassButtonBackground"), AuditionPassButtonSurfacePath, TEXT("PassButtonIcon"), AuditionPassArrowIconPath, TEXT("PASS"), FMargin(282.f, 1122.f, 198.f, 68.f), FVector2D(198.f, 68.f));

        RegisterWidgetVariableGuids(WidgetBlueprint);
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

        if (UTexture2D* DefaultPortrait = LoadObject<UTexture2D>(nullptr, AuditionDefaultPortraitPath))
        {
            SetBlueprintObjectDefault(WidgetBlueprint, TEXT("DefaultPortraitTexture"), DefaultPortrait);
        }

        UPackage* Package = WidgetBlueprint->GetOutermost();
        Package->MarkPackageDirty();
        const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        const bool bSaved = UPackage::SavePackage(Package, WidgetBlueprint, *PackageFilename, FSavePackageArgs());
        UE_LOG(LogTemp, Log, TEXT("RebuildAuditionPanelBlueprint: Rebuilt %s Saved=%s."), AuditionBlueprintPath, bSaved ? TEXT("true") : TEXT("false"));
        return bSaved;
    }

    UButton* AddStudioButton(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* ButtonName, const FString& Label, const FMargin& Offsets, const FLinearColor& LabelColor = TextIvory)
    {
        UButton* Button = ConstructWidget<UButton>(Tree, ButtonName);
        Button->SetBackgroundColor(FLinearColor(0.07f, 0.052f, 0.024f, 0.96f));
        AddToCanvas(Canvas, Button, FAnchors(0.f, 0.f), Offsets);

        UTextBlock* Text = ConstructWidget<UTextBlock>(Tree, *FString::Printf(TEXT("%sText"), ButtonName));
        ConfigureText(Text, Label, 15, LabelColor);
        Text->SetJustification(ETextJustify::Center);
        Button->AddChild(Text);
        return Button;
    }

    bool RebuildRecordSongListItemBlueprint(const TCHAR* BlueprintPath, const TCHAR* PackagePath, const TCHAR* AssetName, bool bSelectedList)
    {
        UWidgetBlueprint* WidgetBlueprint = LoadOrCreateWidgetBlueprint(
            BlueprintPath,
            PackagePath,
            AssetName,
            URecordSongListItemWidget::StaticClass());
        if (!WidgetBlueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildRecordSongListItemBlueprint: Could not load %s."), BlueprintPath);
            return false;
        }

        if (WidgetBlueprint->ParentClass != URecordSongListItemWidget::StaticClass())
        {
            WidgetBlueprint->ParentClass = URecordSongListItemWidget::StaticClass();
        }

        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildRecordSongListItemBlueprint: WidgetTree is missing."));
            return false;
        }

        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        const float Width = bSelectedList ? 666.f : 790.f;
        USizeBox* RootSizeBox = ConstructWidget<USizeBox>(Tree, TEXT("RootSizeBox"), false);
        RootSizeBox->SetWidthOverride(Width);
        RootSizeBox->SetHeightOverride(64.f);
        Tree->RootWidget = RootSizeBox;

        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"), false);
        RootSizeBox->AddChild(RootCanvas);

        UBorder* RowBorder = ConstructWidget<UBorder>(Tree, TEXT("RowBorder"), false);
        RowBorder->SetBrushColor(FLinearColor(0.038f, 0.036f, 0.031f, 0.98f));
        RowBorder->SetPadding(FMargin(0.f));
        AddToCanvas(RootCanvas, RowBorder, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f, 4.f, 0.f, 4.f));

        auto AddLine = [Tree, RootCanvas](const TCHAR* Name, const FMargin& Offsets, const FLinearColor& Color)
        {
            UImage* Line = ConstructWidget<UImage>(Tree, Name, false);
            Line->SetColorAndOpacity(Color);
            AddToCanvas(RootCanvas, Line, FAnchors(0.f, 0.f), Offsets);
        };
        AddLine(TEXT("RowTopLine"), FMargin(0.f, 4.f, Width, 1.f), BorderGold);
        AddLine(TEXT("RowBottomLine"), FMargin(0.f, 59.f, Width, 1.f), FLinearColor(0.20f, 0.18f, 0.13f, 0.48f));
        AddLine(TEXT("GoldAccent"), FMargin(0.f, 8.f, 2.f, 48.f), Gold);

        UButton* PlayButton = AddStudioButton(Tree, RootCanvas, TEXT("PlayButton"), TEXT("PLAY"), FMargin(14.f, 16.f, 44.f, 32.f), Gold);
        PlayButton->SetToolTipText(FText::FromString(TEXT("Preview song")));

        if (!bSelectedList)
        {
            UImage* AlbumThumb = ConstructWidget<UImage>(Tree, TEXT("AlbumThumbImage"), false);
            ConfigureImage(AlbumThumb, AuditionRecordIconPath, FVector2D(42.f, 42.f));
            AddToCanvas(RootCanvas, AlbumThumb, FAnchors(0.f, 0.f), FMargin(70.f, 11.f, 42.f, 42.f));

            UTextBlock* SongNameText = ConstructWidget<UTextBlock>(Tree, TEXT("SongNameText"));
            ConfigureText(SongNameText, TEXT("Song Name"), 12, TextIvory);
            AddToCanvas(RootCanvas, SongNameText, FAnchors(0.f, 0.f), FMargin(128.f, 13.f, 210.f, 20.f));

            UTextBlock* SongMetadataText = ConstructWidget<UTextBlock>(Tree, TEXT("SongMetadataText"));
            ConfigureText(SongMetadataText, TEXT("Genre - Year - Preview"), 9, MutedText);
            AddToCanvas(RootCanvas, SongMetadataText, FAnchors(0.f, 0.f), FMargin(128.f, 35.f, 210.f, 16.f));

            UTextBlock* GenreColumnText = ConstructWidget<UTextBlock>(Tree, TEXT("GenreColumnText"));
            ConfigureText(GenreColumnText, TEXT("Rock and Roll"), 10, MutedText);
            AddToCanvas(RootCanvas, GenreColumnText, FAnchors(0.f, 0.f), FMargin(365.f, 24.f, 110.f, 18.f));

            UTextBlock* DurationColumnText = ConstructWidget<UTextBlock>(Tree, TEXT("DurationColumnText"));
            ConfigureText(DurationColumnText, TEXT("--:--"), 10, TextIvory);
            AddToCanvas(RootCanvas, DurationColumnText, FAnchors(0.f, 0.f), FMargin(500.f, 24.f, 60.f, 18.f));

            UMusicSegmentedMeterWidget* PopularityMeter = ConstructWidget<UMusicSegmentedMeterWidget>(Tree, TEXT("PopularityMeter"));
            PopularityMeter->SetPercent(0.68f);
            AddToCanvas(RootCanvas, PopularityMeter, FAnchors(0.f, 0.f), FMargin(602.f, 24.f, 92.f, 18.f));

            AddStudioButton(Tree, RootCanvas, TEXT("AddButton"), TEXT(""), FMargin(738.f, 19.f, 30.f, 30.f), Gold)->SetToolTipText(FText::FromString(TEXT("Select song")));
            AddCanvasText(Tree, RootCanvas, TEXT("AddButtonText"), TEXT(""), 10, Gold, FMargin(742.f, 23.f, 22.f, 20.f));

            UButton* RemoveButton = AddStudioButton(Tree, RootCanvas, TEXT("RemoveButton"), TEXT("X"), FMargin(Width - 48.f, 18.f, 32.f, 32.f), Gold);
            RemoveButton->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            AddCanvasText(Tree, RootCanvas, TEXT("SelectedTrackIndexPreview"), TEXT("1"), 10, MutedText, FMargin(22.f, 22.f, 30.f, 20.f), false);
            AddCanvasText(Tree, RootCanvas, TEXT("DragHandlePreview"), TEXT("="), 11, MutedText, FMargin(64.f, 22.f, 24.f, 20.f), false);

            UImage* AlbumThumb = ConstructWidget<UImage>(Tree, TEXT("AlbumThumbImage"), false);
            ConfigureImage(AlbumThumb, AuditionRecordIconPath, FVector2D(42.f, 42.f));
            AddToCanvas(RootCanvas, AlbumThumb, FAnchors(0.f, 0.f), FMargin(100.f, 11.f, 42.f, 42.f));

            UTextBlock* SongNameText = ConstructWidget<UTextBlock>(Tree, TEXT("SongNameText"));
            ConfigureText(SongNameText, TEXT("Song Name"), 12, TextIvory);
            AddToCanvas(RootCanvas, SongNameText, FAnchors(0.f, 0.f), FMargin(156.f, 14.f, 260.f, 20.f));

            UTextBlock* SongMetadataText = ConstructWidget<UTextBlock>(Tree, TEXT("SongMetadataText"));
            ConfigureText(SongMetadataText, TEXT("Genre - Year - Preview"), 9, MutedText);
            AddToCanvas(RootCanvas, SongMetadataText, FAnchors(0.f, 0.f), FMargin(156.f, 36.f, 260.f, 16.f));

            UTextBlock* DurationColumnText = ConstructWidget<UTextBlock>(Tree, TEXT("DurationColumnText"));
            ConfigureText(DurationColumnText, TEXT("--:--"), 10, TextIvory);
            AddToCanvas(RootCanvas, DurationColumnText, FAnchors(0.f, 0.f), FMargin(500.f, 24.f, 70.f, 18.f));

            UButton* AddButton = AddStudioButton(Tree, RootCanvas, TEXT("AddButton"), TEXT(""), FMargin(Width - 48.f, 18.f, 32.f, 32.f), Gold);
            AddButton->SetVisibility(ESlateVisibility::Collapsed);
            UButton* RemoveButton = AddStudioButton(Tree, RootCanvas, TEXT("RemoveButton"), TEXT("X"), FMargin(Width - 48.f, 18.f, 32.f, 32.f), Gold);
            RemoveButton->SetToolTipText(FText::FromString(TEXT("Remove song")));
        }

        RegisterWidgetVariableGuids(WidgetBlueprint);
        SaveAndCompileWidgetBlueprint(WidgetBlueprint, TEXT("RebuildRecordSongListItemBlueprint"));
        return true;
    }

    bool RebuildStudioRecordingBlueprint()
    {
        UWidgetBlueprint* WidgetBlueprint = LoadOrCreateWidgetBlueprint(
            RecordingBlueprintPath,
            RecordingBlueprintPackagePath,
            TEXT("RecordingGUIBP"),
            URecordWidget::StaticClass());
        UWidgetBlueprint* AvailableItemBlueprint = LoadObject<UWidgetBlueprint>(nullptr, RecordSongListBlueprintPath);
        UWidgetBlueprint* SelectedItemBlueprint = LoadObject<UWidgetBlueprint>(nullptr, RecordingRecordListBlueprintPath);
        if (!WidgetBlueprint || !AvailableItemBlueprint || !SelectedItemBlueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildStudioRecordingBlueprint: Could not load required Blueprints."));
            return false;
        }

        if (WidgetBlueprint->ParentClass != URecordWidget::StaticClass())
        {
            WidgetBlueprint->ParentClass = URecordWidget::StaticClass();
        }

        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildStudioRecordingBlueprint: WidgetTree is missing."));
            return false;
        }

        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        USizeBox* RootSizeBox = ConstructWidget<USizeBox>(Tree, TEXT("RootSizeBox"), false);
        RootSizeBox->SetWidthOverride(1600.f);
        RootSizeBox->SetHeightOverride(820.f);
        RootSizeBox->SetClipping(EWidgetClipping::ClipToBounds);
        Tree->RootWidget = RootSizeBox;

        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"), false);
        RootSizeBox->AddChild(RootCanvas);

        auto AddLine = [Tree, RootCanvas](const TCHAR* Name, const FMargin& Offsets, const FLinearColor& Color)
        {
            UImage* Line = ConstructWidget<UImage>(Tree, Name, false);
            Line->SetColorAndOpacity(Color);
            AddToCanvas(RootCanvas, Line, FAnchors(0.f, 0.f), Offsets);
        };
        auto AddOutline = [&AddLine](const TCHAR* Prefix, const FMargin& Rect, const FLinearColor& Color)
        {
            AddLine(*FString::Printf(TEXT("%sTop"), Prefix), FMargin(Rect.Left, Rect.Top, Rect.Right, 1.f), Color);
            AddLine(*FString::Printf(TEXT("%sBottom"), Prefix), FMargin(Rect.Left, Rect.Top + Rect.Bottom - 1.f, Rect.Right, 1.f), Color);
            AddLine(*FString::Printf(TEXT("%sLeft"), Prefix), FMargin(Rect.Left, Rect.Top, 1.f, Rect.Bottom), Color);
            AddLine(*FString::Printf(TEXT("%sRight"), Prefix), FMargin(Rect.Left + Rect.Right - 1.f, Rect.Top, 1.f, Rect.Bottom), Color);
        };
        auto AddDottedRow = [&AddLine](const TCHAR* Prefix, float X, float Y, float W, float H)
        {
            const float Dash = 18.f;
            const float Gap = 10.f;
            int32 Index = 0;
            for (float Dx = 0.f; Dx < W; Dx += Dash + Gap)
            {
                AddLine(*FString::Printf(TEXT("%sTopDash%d"), Prefix, Index), FMargin(X + Dx, Y, FMath::Min(Dash, W - Dx), 1.f), BorderGold);
                AddLine(*FString::Printf(TEXT("%sBottomDash%d"), Prefix, Index), FMargin(X + Dx, Y + H, FMath::Min(Dash, W - Dx), 1.f), BorderGold);
                ++Index;
            }
            for (float Dy = 0.f; Dy < H; Dy += Dash + Gap)
            {
                AddLine(*FString::Printf(TEXT("%sLeftDash%d"), Prefix, Index), FMargin(X, Y + Dy, 1.f, FMath::Min(Dash, H - Dy)), BorderGold);
                AddLine(*FString::Printf(TEXT("%sRightDash%d"), Prefix, Index), FMargin(X + W, Y + Dy, 1.f, FMath::Min(Dash, H - Dy)), BorderGold);
                ++Index;
            }
        };

        UBorder* Background = ConstructWidget<UBorder>(Tree, TEXT("StudioPanelBackground"), false);
        Background->SetBrushColor(FLinearColor(0.010f, 0.011f, 0.010f, 0.99f));
        Background->SetPadding(FMargin(0.f));
        AddToCanvas(RootCanvas, Background, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));
        AddCanvasImage(Tree, RootCanvas, TEXT("StudioVinylTexture"), TopStatusBarSurfacePath, FMargin(0.f, 0.f, 1600.f, 820.f), FVector2D(1600.f, 820.f), false);

        AddIconToCanvas(Tree, RootCanvas, TEXT("StudioHeaderRecordIcon"), AuditionRecordIconPath, FMargin(24.f, 16.f, 42.f, 42.f), 42.f);
        AddCanvasText(Tree, RootCanvas, TEXT("StudioHeaderText"), TEXT("STUDIO RECORDING"), 29, Gold, FMargin(82.f, 16.f, 390.f, 42.f), false);
        UButton* CloseButton = AddStudioButton(Tree, RootCanvas, TEXT("CloseButton"), TEXT("X"), FMargin(1540.f, 14.f, 38.f, 38.f), Gold);
        CloseButton->SetToolTipText(FText::FromString(TEXT("Close studio")));
        AddLine(TEXT("HeaderGoldRule"), FMargin(24.f, 72.f, 1552.f, 1.f), BorderGold);

        AddCanvasImage(Tree, RootCanvas, TEXT("ArtistPortraitImage"), AuditionDefaultPortraitPath, FMargin(36.f, 82.f, 126.f, 126.f), FVector2D(126.f, 126.f), false);
        AddCanvasImage(Tree, RootCanvas, TEXT("ArtistVinylFrame"), AuditionVinylFramePath, FMargin(18.f, 64.f, 162.f, 162.f), FVector2D(162.f, 162.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("ArtistNameText"), TEXT("Artist"), 21, Gold, FMargin(200.f, 86.f, 210.f, 30.f));
        UBorder* Badge = ConstructWidget<UBorder>(Tree, TEXT("SignedArtistBadgeSurface"), false);
        Badge->SetBrushColor(FLinearColor(0.050f, 0.038f, 0.018f, 0.96f));
        Badge->SetPadding(FMargin(0.f));
        AddToCanvas(RootCanvas, Badge, FAnchors(0.f, 0.f), FMargin(364.f, 92.f, 86.f, 24.f));
        AddOutline(TEXT("SignedBadgeBorder"), FMargin(364.f, 92.f, 86.f, 24.f), BorderGold);
        AddCanvasText(Tree, RootCanvas, TEXT("SignedArtistBadge"), TEXT("SIGNED ARTIST"), 8, Gold, FMargin(372.f, 97.f, 70.f, 14.f), false);
        AddIconToCanvas(Tree, RootCanvas, TEXT("ArtistGenreIcon"), AuditionSongwritingIconPath, FMargin(200.f, 124.f, 18.f, 18.f), 18.f);
        AddCanvasText(Tree, RootCanvas, TEXT("ArtistGenreText"), TEXT("Genre"), 12, TextIvory, FMargin(224.f, 122.f, 220.f, 22.f));
        AddCanvasText(Tree, RootCanvas, TEXT("PopularityLabel"), TEXT("Popularity"), 9, MutedText, FMargin(200.f, 158.f, 78.f, 16.f), false);
        AddIconToCanvas(Tree, RootCanvas, TEXT("PopularityValueIcon"), AuditionAudienceIconPath, FMargin(200.f, 178.f, 16.f, 16.f), 16.f);
        AddCanvasText(Tree, RootCanvas, TEXT("ArtistPopularityText"), TEXT("--"), 10, TextIvory, FMargin(222.f, 176.f, 58.f, 18.f));
        AddCanvasText(Tree, RootCanvas, TEXT("FansLabel"), TEXT("Fans"), 9, MutedText, FMargin(300.f, 158.f, 64.f, 16.f), false);
        AddIconToCanvas(Tree, RootCanvas, TEXT("FansValueIcon"), AuditionAudienceIconPath, FMargin(300.f, 178.f, 16.f, 16.f), 16.f);
        AddCanvasText(Tree, RootCanvas, TEXT("ArtistFansText"), TEXT("--"), 10, TextIvory, FMargin(322.f, 176.f, 70.f, 18.f));
        AddCanvasText(Tree, RootCanvas, TEXT("ReputationLabel"), TEXT("Reputation"), 9, MutedText, FMargin(402.f, 158.f, 90.f, 16.f), false);
        AddIconToCanvas(Tree, RootCanvas, TEXT("ReputationValueIcon"), ReputationIconPath, FMargin(402.f, 178.f, 16.f, 16.f), 16.f);
        AddCanvasText(Tree, RootCanvas, TEXT("ArtistReputationText"), TEXT("--"), 10, TextIvory, FMargin(424.f, 176.f, 58.f, 18.f));

        const float FormatY = 96.f;
        auto AddFormatCard = [Tree, RootCanvas, FormatY, &AddOutline](const TCHAR* CheckName, const TCHAR* LabelName, const TCHAR* Label, float X, bool bChecked)
        {
            UBorder* Card = ConstructWidget<UBorder>(Tree, *FString::Printf(TEXT("%sCard"), CheckName), false);
            Card->SetBrushColor(bChecked ? FLinearColor(0.112f, 0.083f, 0.033f, 0.98f) : FLinearColor(0.044f, 0.043f, 0.039f, 0.96f));
            Card->SetPadding(FMargin(0.f));
            AddToCanvas(RootCanvas, Card, FAnchors(0.f, 0.f), FMargin(X, FormatY, 190.f, 76.f));
            AddOutline(*FString::Printf(TEXT("%sOutline"), CheckName), FMargin(X, FormatY, 190.f, 76.f), bChecked ? Gold : FLinearColor(0.22f, 0.20f, 0.16f, 0.80f));
            UCheckBox* Check = ConstructWidget<UCheckBox>(Tree, CheckName);
            Check->SetIsChecked(bChecked);
            AddToCanvas(RootCanvas, Check, FAnchors(0.f, 0.f), FMargin(X + 16.f, FormatY + 27.f, 20.f, 20.f));
            AddIconToCanvas(Tree, RootCanvas, *FString::Printf(TEXT("%sIcon"), CheckName), AuditionRecordIconPath, FMargin(X + 50.f, FormatY + 17.f, 42.f, 42.f), 42.f);
            AddCanvasText(Tree, RootCanvas, LabelName, Label, 15, TextIvory, FMargin(X + 112.f, FormatY + 25.f, 60.f, 24.f), false);
        };
        AddFormatCard(TEXT("bIsSingle"), TEXT("SingleLabel"), TEXT("SINGLE"), 540.f, true);
        AddFormatCard(TEXT("bIsEP"), TEXT("EPLabel"), TEXT("EP"), 750.f, false);
        AddFormatCard(TEXT("bIsLP"), TEXT("LPLabel"), TEXT("LP"), 960.f, false);
        AddCanvasText(Tree, RootCanvas, TEXT("FormatHelpText"), TEXT("1-2 tracks - Shorter release - Lower cost"), 10, MutedText, FMargin(654.f, 184.f, 420.f, 18.f), false);

        UBorder* OverviewPanel = ConstructWidget<UBorder>(Tree, TEXT("ReleaseOverviewPanel"), false);
        OverviewPanel->SetBrushColor(FLinearColor(0.024f, 0.023f, 0.020f, 0.97f));
        AddToCanvas(RootCanvas, OverviewPanel, FAnchors(0.f, 0.f), FMargin(1288.f, 82.f, 286.f, 126.f));
        AddOutline(TEXT("ReleaseOverviewOutline"), FMargin(1288.f, 82.f, 286.f, 126.f), FLinearColor(0.27f, 0.22f, 0.13f, 0.80f));
        AddIconToCanvas(Tree, RootCanvas, TEXT("ReleaseOverviewIcon"), AuditionRecordIconPath, FMargin(1310.f, 102.f, 28.f, 28.f), 28.f);
        AddLine(TEXT("ReleaseOverviewGoldStem"), FMargin(1324.f, 136.f, 1.f, 56.f), Gold);
        AddCanvasText(Tree, RootCanvas, TEXT("ReleaseOverviewHeader"), TEXT("RELEASE OVERVIEW"), 13, TextIvory, FMargin(1352.f, 100.f, 180.f, 22.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("FormatOverviewLabel"), TEXT("Format"), 10, MutedText, FMargin(1352.f, 134.f, 80.f, 18.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("ReleaseFormatText"), TEXT("Single"), 10, TextIvory, FMargin(1500.f, 134.f, 70.f, 18.f));
        AddCanvasText(Tree, RootCanvas, TEXT("TracksOverviewLabel"), TEXT("Tracks"), 10, MutedText, FMargin(1352.f, 158.f, 80.f, 18.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("ReleaseTracksText"), TEXT("0 / 2"), 10, TextIvory, FMargin(1500.f, 158.f, 70.f, 18.f));
        AddCanvasText(Tree, RootCanvas, TEXT("DurationOverviewLabel"), TEXT("Total Duration"), 10, MutedText, FMargin(1352.f, 182.f, 100.f, 18.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("ReleaseDurationText"), TEXT("--:--"), 10, TextIvory, FMargin(1500.f, 182.f, 70.f, 18.f));

        UBorder* AvailablePanel = ConstructWidget<UBorder>(Tree, TEXT("AvailableSongsPanel"), false);
        AvailablePanel->SetBrushColor(FLinearColor(0.017f, 0.017f, 0.015f, 0.985f));
        AddToCanvas(RootCanvas, AvailablePanel, FAnchors(0.f, 0.f), FMargin(24.f, 232.f, 820.f, 398.f));
        AddOutline(TEXT("AvailableSongsOutline"), FMargin(24.f, 232.f, 820.f, 398.f), FLinearColor(0.24f, 0.21f, 0.15f, 0.70f));
        AddCanvasText(Tree, RootCanvas, TEXT("AvailableSongsHeader"), TEXT("AVAILABLE SONGS"), 16, Gold, FMargin(42.f, 252.f, 180.f, 24.f), false);
        UComboBoxString* GenreFilterBox = ConstructWidget<UComboBoxString>(Tree, TEXT("GenreFilterBox"));
        GenreFilterBox->AddOption(TEXT("All Genres"));
        GenreFilterBox->SetSelectedOption(TEXT("All Genres"));
        AddToCanvas(RootCanvas, GenreFilterBox, FAnchors(0.f, 0.f), FMargin(204.f, 248.f, 132.f, 30.f));
        UEditableTextBox* SearchTextBox = ConstructWidget<UEditableTextBox>(Tree, TEXT("SearchTextBox"));
        SearchTextBox->SetHintText(FText::FromString(TEXT("Search songs...")));
        AddToCanvas(RootCanvas, SearchTextBox, FAnchors(0.f, 0.f), FMargin(350.f, 248.f, 286.f, 30.f));
        AddIconToCanvas(Tree, RootCanvas, TEXT("SearchIcon"), StudioSearchIconPath, FMargin(358.f, 254.f, 18.f, 18.f), 18.f);
        AddIconToCanvas(Tree, RootCanvas, TEXT("FilterIcon"), StudioFilterIconPath, FMargin(650.f, 249.f, 28.f, 28.f), 28.f);
        AddCanvasText(Tree, RootCanvas, TEXT("TitleColumnHeader"), TEXT("TITLE"), 9, MutedText, FMargin(96.f, 304.f, 80.f, 16.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("GenreColumnHeader"), TEXT("GENRE"), 9, MutedText, FMargin(365.f, 304.f, 80.f, 16.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("DurationColumnHeader"), TEXT("DURATION"), 9, MutedText, FMargin(500.f, 304.f, 80.f, 16.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("PopularityColumnHeader"), TEXT("POPULARITY"), 9, MutedText, FMargin(602.f, 304.f, 100.f, 16.f), false);
        UListView* SongListView = ConstructWidget<UListView>(Tree, TEXT("SongListView"));
        SetListViewEntryWidgetClass(SongListView, AvailableItemBlueprint->GeneratedClass);
        AddToCanvas(RootCanvas, SongListView, FAnchors(0.f, 0.f), FMargin(42.f, 332.f, 790.f, 260.f));

        UBorder* SelectedPanel = ConstructWidget<UBorder>(Tree, TEXT("SelectedTracksPanel"), false);
        SelectedPanel->SetBrushColor(FLinearColor(0.017f, 0.017f, 0.015f, 0.985f));
        AddToCanvas(RootCanvas, SelectedPanel, FAnchors(0.f, 0.f), FMargin(864.f, 232.f, 708.f, 398.f));
        AddOutline(TEXT("SelectedTracksOutline"), FMargin(864.f, 232.f, 708.f, 398.f), FLinearColor(0.24f, 0.21f, 0.15f, 0.70f));
        AddCanvasText(Tree, RootCanvas, TEXT("SelectedTracksHeader"), TEXT("SELECTED TRACKS"), 16, Gold, FMargin(884.f, 252.f, 220.f, 24.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("SelectedTrackCountText"), TEXT("0 / 2 TRACKS"), 11, Gold, FMargin(1452.f, 254.f, 110.f, 20.f));
        AddCanvasText(Tree, RootCanvas, TEXT("SelectedIndexHeader"), TEXT("#"), 9, MutedText, FMargin(904.f, 304.f, 30.f, 16.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("SelectedTitleHeader"), TEXT("TITLE"), 9, MutedText, FMargin(982.f, 304.f, 80.f, 16.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("SelectedDurationHeader"), TEXT("DURATION"), 9, MutedText, FMargin(1364.f, 304.f, 90.f, 16.f), false);
        AddIconToCanvas(Tree, RootCanvas, TEXT("SelectedDurationClockIcon"), StudioClockIconPath, FMargin(1482.f, 302.f, 16.f, 16.f), 16.f);
        UListView* RecordSongListView = ConstructWidget<UListView>(Tree, TEXT("RecordSongListView"));
        SetListViewEntryWidgetClass(RecordSongListView, SelectedItemBlueprint->GeneratedClass);
        AddToCanvas(RootCanvas, RecordSongListView, FAnchors(0.f, 0.f), FMargin(884.f, 332.f, 666.f, 92.f));
        AddDottedRow(TEXT("AddTrackPlaceholderBorder"), 884.f, 434.f, 666.f, 56.f);
        AddCanvasText(Tree, RootCanvas, TEXT("AddTrackIndexPreview"), TEXT("2"), 12, MutedText, FMargin(910.f, 450.f, 30.f, 22.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("AddTrackPlaceholder"), TEXT("+  Add a track"), 13, MutedText, FMargin(1128.f, 450.f, 220.f, 24.f), false);

        UBorder* SummaryPanel = ConstructWidget<UBorder>(Tree, TEXT("ProductionSummaryPanel"), false);
        SummaryPanel->SetBrushColor(FLinearColor(0.024f, 0.023f, 0.020f, 0.985f));
        AddToCanvas(RootCanvas, SummaryPanel, FAnchors(0.f, 1.f, 1.f, 1.f), FMargin(24.f, -102.f, 24.f, 84.f));
        AddOutline(TEXT("SummaryPanelOutline"), FMargin(24.f, 704.f, 1552.f, 92.f), FLinearColor(0.25f, 0.21f, 0.13f, 0.75f));
        AddIconToCanvas(Tree, RootCanvas, TEXT("CostIcon"), CashIconPath, FMargin(48.f, 724.f, 46.f, 46.f), 46.f);
        AddCanvasText(Tree, RootCanvas, TEXT("CostLabel"), TEXT("RECORDING COST"), 10, MutedText, FMargin(108.f, 724.f, 150.f, 18.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("RecordingCostText"), TEXT("$0"), 18, TextIvory, FMargin(108.f, 746.f, 170.f, 28.f));
        AddLine(TEXT("SummaryDividerA"), FMargin(254.f, 722.f, 1.f, 54.f), FLinearColor(0.30f, 0.25f, 0.15f, 0.55f));
        AddIconToCanvas(Tree, RootCanvas, TEXT("CompletionIcon"), DateIconPath, FMargin(292.f, 724.f, 42.f, 42.f), 42.f);
        AddCanvasText(Tree, RootCanvas, TEXT("DurationLabel"), TEXT("COMPLETION"), 10, MutedText, FMargin(354.f, 724.f, 130.f, 18.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("RecordingDurationText"), TEXT("--"), 17, TextIvory, FMargin(354.f, 746.f, 180.f, 28.f));
        AddLine(TEXT("SummaryDividerB"), FMargin(530.f, 722.f, 1.f, 54.f), FLinearColor(0.30f, 0.25f, 0.15f, 0.55f));
        AddIconToCanvas(Tree, RootCanvas, TEXT("TotalDurationIcon"), StudioClockIconPath, FMargin(566.f, 724.f, 42.f, 42.f), 42.f);
        AddCanvasText(Tree, RootCanvas, TEXT("TotalDurationLabel"), TEXT("TOTAL DURATION"), 10, MutedText, FMargin(628.f, 724.f, 150.f, 18.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("TotalDurationText"), TEXT("--:--"), 17, TextIvory, FMargin(628.f, 746.f, 120.f, 28.f));
        AddLine(TEXT("SummaryDividerC"), FMargin(770.f, 722.f, 1.f, 54.f), FLinearColor(0.30f, 0.25f, 0.15f, 0.55f));
        AddIconToCanvas(Tree, RootCanvas, TEXT("WarningIcon"), StudioWarningIconPath, FMargin(804.f, 728.f, 34.f, 34.f), 34.f);
        AddCanvasText(Tree, RootCanvas, TEXT("WarningsLabel"), TEXT("WARNINGS"), 10, MutedText, FMargin(850.f, 724.f, 110.f, 18.f), false);
        UTextBlock* WarningText = AddCanvasText(Tree, RootCanvas, TEXT("RecordingWarningText"), TEXT("Select songs to see validation."), 9, MutedText, FMargin(850.f, 746.f, 250.f, 36.f));
        WarningText->SetAutoWrapText(true);
        AddStudioButton(Tree, RootCanvas, TEXT("ConfirmButton"), TEXT("CONFIRM RECORDING"), FMargin(1122.f, 724.f, 248.f, 58.f), Gold);
        AddStudioButton(Tree, RootCanvas, TEXT("CancelButton"), TEXT("CANCEL"), FMargin(1392.f, 724.f, 160.f, 58.f), TextIvory);

        UEditableTextBox* AlbumNameBox = ConstructWidget<UEditableTextBox>(Tree, TEXT("AlbumNameBox"));
        AlbumNameBox->SetHintText(FText::FromString(TEXT("Record title")));
        AddToCanvas(RootCanvas, AlbumNameBox, FAnchors(0.f, 0.f), FMargin(1168.f, 22.f, 300.f, 34.f));

        RegisterWidgetVariableGuids(WidgetBlueprint);
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);

        UPackage* Package = WidgetBlueprint->GetOutermost();
        Package->MarkPackageDirty();
        const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        const bool bSaved = UPackage::SavePackage(Package, WidgetBlueprint, *PackageFilename, FSavePackageArgs());
        UE_LOG(LogTemp, Log, TEXT("RebuildStudioRecordingBlueprint: Rebuilt %s Saved=%s."), RecordingBlueprintPath, bSaved ? TEXT("true") : TEXT("false"));
        return bSaved;
    }
    bool RebuildCommandItemBlueprint()
    {
        UWidgetBlueprint* WidgetBlueprint = LoadOrCreateWidgetBlueprint(
            CommandItemBlueprintPath,
            CommandItemBlueprintPackagePath,
            TEXT("CommandItemWidgetBP"),
            UCommandItemWidget::StaticClass());
        if (!WidgetBlueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildCommandItemBlueprint: Could not load or create %s."), CommandItemBlueprintPath);
            return false;
        }

        if (WidgetBlueprint->ParentClass != UCommandItemWidget::StaticClass())
        {
            WidgetBlueprint->ParentClass = UCommandItemWidget::StaticClass();
        }

        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildCommandItemBlueprint: WidgetTree is missing."));
            return false;
        }

        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        USizeBox* RootSizeBox = ConstructWidget<USizeBox>(Tree, TEXT("RootSizeBox"), false);
        RootSizeBox->SetWidthOverride(150.f);
        RootSizeBox->SetHeightOverride(122.f);
        Tree->RootWidget = RootSizeBox;

        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"), false);
        RootSizeBox->AddChild(RootCanvas);

        UBorder* BackgroundBorder = ConstructWidget<UBorder>(Tree, TEXT("BackgroundBorder"));
        BackgroundBorder->SetBrushColor(FLinearColor(0.035f, 0.034f, 0.028f, 1.f));
        AddToCanvas(RootCanvas, BackgroundBorder, FAnchors(0.f, 0.f), FMargin(0.f, 0.f, 150.f, 122.f));

        UImage* ButtonSurface = ConstructWidget<UImage>(Tree, TEXT("ButtonSurfaceImage"), false);
        ConfigureImage(ButtonSurface, CommandDockButtonPath, FVector2D(150.f, 116.f));
        AddToCanvas(RootCanvas, ButtonSurface, FAnchors(0.f, 0.f), FMargin(0.f, 0.f, 150.f, 116.f));

        UBorder* OutlineBorder = ConstructWidget<UBorder>(Tree, TEXT("OutlineBorder"));
        OutlineBorder->SetBrushColor(FLinearColor::Transparent);
        AddToCanvas(RootCanvas, OutlineBorder, FAnchors(0.f, 0.f), FMargin(0.f, 0.f, 150.f, 122.f));

        UImage* CommandImage = ConstructWidget<UImage>(Tree, TEXT("CommandImage"));
        CommandImage->SetColorAndOpacity(FLinearColor::White);
        AddToCanvas(RootCanvas, CommandImage, FAnchors(0.f, 0.f), FMargin(45.f, 15.f, 60.f, 60.f));

        UTextBlock* CommandLabelText = ConstructWidget<UTextBlock>(Tree, TEXT("CommandLabelText"));
        ConfigureText(CommandLabelText, TEXT("Command"), 12, Gold);
        CommandLabelText->SetJustification(ETextJustify::Center);
        AddToCanvas(RootCanvas, CommandLabelText, FAnchors(0.f, 0.f), FMargin(10.f, 82.f, 130.f, 26.f));

        UButton* CommandButton = ConstructWidget<UButton>(Tree, TEXT("CommandButton"));
        CommandButton->SetBackgroundColor(Transparent);
        AddToCanvas(RootCanvas, CommandButton, FAnchors(0.f, 0.f), FMargin(0.f, 0.f, 150.f, 122.f));

        RegisterWidgetVariableGuids(WidgetBlueprint);
        SaveAndCompileWidgetBlueprint(WidgetBlueprint, TEXT("RebuildCommandItemBlueprint"));
        return true;
    }

    bool RebuildCommandPanelBlueprint()
    {
        UWidgetBlueprint* WidgetBlueprint = LoadOrCreateWidgetBlueprint(
            CommandPanelBlueprintPath,
            CommandPanelBlueprintPackagePath,
            TEXT("CommandPanelBP"),
            UCommandPanelWidget::StaticClass());
        UWidgetBlueprint* ItemBlueprint = LoadObject<UWidgetBlueprint>(nullptr, CommandItemBlueprintPath);
        if (!WidgetBlueprint || !ItemBlueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildCommandPanelBlueprint: Could not load panel or item Blueprint."));
            return false;
        }

        if (WidgetBlueprint->ParentClass != UCommandPanelWidget::StaticClass())
        {
            WidgetBlueprint->ParentClass = UCommandPanelWidget::StaticClass();
        }

        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree)
        {
            UE_LOG(LogTemp, Error, TEXT("RebuildCommandPanelBlueprint: WidgetTree is missing."));
            return false;
        }

        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        USizeBox* RootSizeBox = ConstructWidget<USizeBox>(Tree, TEXT("RootSizeBox"), false);
        RootSizeBox->SetWidthOverride(980.f);
        RootSizeBox->SetHeightOverride(150.f);
        Tree->RootWidget = RootSizeBox;

        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"), false);
        RootSizeBox->AddChild(RootCanvas);

        UImage* BackgroundImageSecondary = ConstructWidget<UImage>(Tree, TEXT("BackgroundImageSecondary"));
        ConfigureImage(BackgroundImageSecondary, CommandDockBackgroundPath, FVector2D(980.f, 150.f));
        BackgroundImageSecondary->SetColorAndOpacity(FLinearColor(0.04f, 0.035f, 0.025f, 0.82f));
        AddToCanvas(RootCanvas, BackgroundImageSecondary, FAnchors(0.f, 0.f), FMargin(0.f, 0.f, 980.f, 150.f));

        UImage* BackgroundImagePrimary = ConstructWidget<UImage>(Tree, TEXT("BackgroundImagePrimary"));
        ConfigureImage(BackgroundImagePrimary, CommandDockBackgroundPath, FVector2D(960.f, 138.f));
        AddToCanvas(RootCanvas, BackgroundImagePrimary, FAnchors(0.f, 0.f), FMargin(10.f, 6.f, 960.f, 138.f));

        UHorizontalBox* CommandPanel = ConstructWidget<UHorizontalBox>(Tree, TEXT("CommandPanel"));
        AddToCanvas(RootCanvas, CommandPanel, FAnchors(0.f, 0.f), FMargin(105.f, 15.f, 770.f, 122.f));

        RegisterWidgetVariableGuids(WidgetBlueprint);
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
        SetBlueprintSubclassDefault(WidgetBlueprint, TEXT("CommandItemWidgetClass"), ItemBlueprint->GeneratedClass);
        SaveAndCompileWidgetBlueprint(WidgetBlueprint, TEXT("RebuildCommandPanelBlueprint"));
        return true;
    }
}
    UBorder* AddContractsPanelBorder(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Name, const FMargin& Offsets, const FLinearColor& Color)
    {
        UBorder* Border = ConstructWidget<UBorder>(Tree, Name, false);
        Border->SetBrushColor(Color);
        Border->SetPadding(FMargin(0.f));
        AddToCanvas(Canvas, Border, FAnchors(0.f, 0.f), Offsets);
        return Border;
    }

    void AddContractsLabelValue(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Label, const TCHAR* ValueName, const TCHAR* IconPath, float X, float Y, float Width)
    {
        UImage* Icon = ConstructWidget<UImage>(Tree, *FString::Printf(TEXT("%sIcon"), ValueName), false);
        ConfigureImage(Icon, IconPath, FVector2D(26.f, 26.f));
        AddToCanvas(Canvas, Icon, FAnchors(0.f, 0.f), FMargin(X, Y + 2.f, 26.f, 26.f));
        AddCanvasText(Tree, Canvas, *FString::Printf(TEXT("%sLabel"), ValueName), Label, 13, MutedText, FMargin(X + 34.f, Y, Width - 34.f, 22.f), false);
        AddCanvasText(Tree, Canvas, ValueName, TEXT("--"), 18, TextIvory, FMargin(X + 34.f, Y + 24.f, Width - 34.f, 30.f));
    }

    bool RebuildActiveContractItemBlueprint()
    {
        UWidgetBlueprint* WidgetBlueprint = LoadOrCreateWidgetBlueprint(ActiveContractItemBlueprintPath, ActiveContractItemBlueprintPackagePath, TEXT("ActiveContractItemBP"), UActiveContractItemWidget::StaticClass());
        if (!WidgetBlueprint) return false;
        if (WidgetBlueprint->ParentClass != UActiveContractItemWidget::StaticClass()) WidgetBlueprint->ParentClass = UActiveContractItemWidget::StaticClass();
        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree) return false;
        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        USizeBox* RootSize = ConstructWidget<USizeBox>(Tree, TEXT("RootSize"), false);
        RootSize->SetWidthOverride(480.f);
        RootSize->SetHeightOverride(126.f);
        Tree->RootWidget = RootSize;
        UButton* ContractButton = ConstructWidget<UButton>(Tree, TEXT("ContractButton"));
        ContractButton->SetBackgroundColor(FLinearColor(0.045f, 0.041f, 0.034f, 0.98f));
        RootSize->AddChild(ContractButton);
        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"), false);
        ContractButton->AddChild(RootCanvas);
        AddContractsPanelBorder(Tree, RootCanvas, TEXT("CardEdge"), FMargin(0.f, 0.f, 480.f, 126.f), FLinearColor(0.66f, 0.45f, 0.13f, 0.28f));
        AddContractsPanelBorder(Tree, RootCanvas, TEXT("CardSurface"), FMargin(2.f, 2.f, 476.f, 122.f), FLinearColor(0.035f, 0.034f, 0.030f, 0.96f));
        AddCanvasImage(Tree, RootCanvas, TEXT("ArtistPortraitImage"), ContractsArtistIconPath, FMargin(18.f, 20.f, 72.f, 72.f), FVector2D(72.f, 72.f));
        AddCanvasText(Tree, RootCanvas, TEXT("ArtistNameText"), TEXT("Artist Name"), 18, Gold, FMargin(104.f, 20.f, 220.f, 28.f));
        AddCanvasText(Tree, RootCanvas, TEXT("GenreText"), TEXT("Genre"), 12, MutedText, FMargin(104.f, 48.f, 190.f, 22.f));
        AddCanvasText(Tree, RootCanvas, TEXT("TermText"), TEXT("0 / 0 months"), 12, TextIvory, FMargin(104.f, 72.f, 170.f, 22.f));
        AddCanvasText(Tree, RootCanvas, TEXT("RecordsText"), TEXT("0 / 0 records"), 12, TextIvory, FMargin(286.f, 72.f, 160.f, 22.f));
        AddCanvasText(Tree, RootCanvas, TEXT("StatusText"), TEXT("ACTIVE"), 12, Gold, FMargin(372.f, 20.f, 86.f, 24.f));
        AddCanvasText(Tree, RootCanvas, TEXT("EconomicsText"), TEXT("Rev $0 Cost $0"), 11, MutedText, FMargin(104.f, 96.f, 260.f, 20.f));
        UProgressBar* TermProgressBar = ConstructWidget<UProgressBar>(Tree, TEXT("TermProgressBar"));
        TermProgressBar->SetPercent(0.4f);
        AddToCanvas(RootCanvas, TermProgressBar, FAnchors(0.f, 0.f), FMargin(286.f, 100.f, 150.f, 10.f));
        SaveAndCompileWidgetBlueprint(WidgetBlueprint, TEXT("RebuildActiveContractItemBlueprint"));
        return true;
    }

    bool RebuildActiveContractsPanelBlueprint()
    {
        UWidgetBlueprint* ItemBlueprint = LoadObject<UWidgetBlueprint>(nullptr, ActiveContractItemBlueprintPath);
        if (!ItemBlueprint || !ItemBlueprint->GeneratedClass) return false;
        UWidgetBlueprint* WidgetBlueprint = LoadOrCreateWidgetBlueprint(ActiveContractsBlueprintPath, ActiveContractsBlueprintPackagePath, TEXT("ActiveContractsBP"), UActiveContractsWidget::StaticClass());
        if (!WidgetBlueprint) return false;
        if (WidgetBlueprint->ParentClass != UActiveContractsWidget::StaticClass()) WidgetBlueprint->ParentClass = UActiveContractsWidget::StaticClass();
        UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
        if (!Tree) return false;
        WidgetBlueprint->Modify();
        Tree->Modify();
        ClearWidgetTree(Tree);

        USizeBox* RootSize = ConstructWidget<USizeBox>(Tree, TEXT("RootSize"), false);
        RootSize->SetWidthOverride(1480.f);
        RootSize->SetHeightOverride(760.f);
        Tree->RootWidget = RootSize;
        UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"), false);
        RootSize->AddChild(RootCanvas);
        AddContractsPanelBorder(Tree, RootCanvas, TEXT("OuterGoldBorder"), FMargin(0.f, 0.f, 1480.f, 760.f), FLinearColor(0.82f, 0.55f, 0.15f, 0.55f));
        AddContractsPanelBorder(Tree, RootCanvas, TEXT("PanelSurface"), FMargin(3.f, 3.f, 1474.f, 754.f), FLinearColor(0.022f, 0.021f, 0.018f, 0.96f));
        AddCanvasImage(Tree, RootCanvas, TEXT("HeaderIconImage"), ContractsContractIconPath, FMargin(34.f, 26.f, 42.f, 42.f), FVector2D(42.f, 42.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("TitleText"), TEXT("ACTIVE CONTRACTS"), 30, Gold, FMargin(90.f, 28.f, 420.f, 48.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("HeaderCountText"), TEXT("0 active"), 15, MutedText, FMargin(438.f, 39.f, 120.f, 28.f));
        UButton* CloseButton = ConstructWidget<UButton>(Tree, TEXT("CloseButton"));
        CloseButton->SetBackgroundColor(FLinearColor(0.08f, 0.055f, 0.025f, 1.f));
        AddToCanvas(RootCanvas, CloseButton, FAnchors(0.f, 0.f), FMargin(1412.f, 24.f, 44.f, 44.f));
        AddButtonIcon(Tree, CloseButton, TEXT("CloseButton"), ContractsCloseIconPath);

        AddContractsPanelBorder(Tree, RootCanvas, TEXT("ListPanelSurface"), FMargin(34.f, 100.f, 510.f, 540.f), FLinearColor(0.055f, 0.052f, 0.044f, 0.90f));
        AddCanvasText(Tree, RootCanvas, TEXT("ListHeaderText"), TEXT("SIGNED DEALS"), 18, Gold, FMargin(60.f, 122.f, 220.f, 28.f), false);
        UScrollBox* ContractScrollBox = ConstructWidget<UScrollBox>(Tree, TEXT("ContractScrollBox"));
        AddToCanvas(RootCanvas, ContractScrollBox, FAnchors(0.f, 0.f), FMargin(50.f, 162.f, 480.f, 440.f));
        AddCanvasText(Tree, RootCanvas, TEXT("EmptyStateText"), TEXT("No active contracts. Sign an artist from audition news to create one."), 16, MutedText, FMargin(70.f, 340.f, 410.f, 72.f));

        AddContractsPanelBorder(Tree, RootCanvas, TEXT("DetailPanelSurface"), FMargin(570.f, 100.f, 876.f, 540.f), FLinearColor(0.055f, 0.052f, 0.044f, 0.90f));
        AddCanvasImage(Tree, RootCanvas, TEXT("DetailArtistIcon"), ContractsArtistIconPath, FMargin(610.f, 132.f, 56.f, 56.f), FVector2D(56.f, 56.f), false);
        AddCanvasText(Tree, RootCanvas, TEXT("DetailArtistNameText"), TEXT("No active contracts"), 28, TextIvory, FMargin(684.f, 126.f, 470.f, 44.f));
        AddCanvasText(Tree, RootCanvas, TEXT("DetailGenreText"), TEXT("Sign an artist to create the first contract."), 15, MutedText, FMargin(688.f, 168.f, 420.f, 28.f));
        AddCanvasText(Tree, RootCanvas, TEXT("DetailStatusText"), TEXT("NO CONTRACT"), 15, Gold, FMargin(1212.f, 138.f, 160.f, 30.f));
        AddContractsLabelValue(Tree, RootCanvas, TEXT("PERIOD"), TEXT("DetailPeriodText"), ContractsCalendarIconPath, 610.f, 222.f, 390.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("TERMS"), TEXT("DetailTermsText"), ContractsRecordIconPath, 1030.f, 222.f, 300.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("ROYALTY"), TEXT("DetailRoyaltyText"), ContractsRoyaltyIconPath, 610.f, 304.f, 220.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("SIGNING BONUS"), TEXT("DetailBonusText"), ContractsBonusIconPath, 860.f, 304.f, 240.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("MONTHLY UPKEEP"), TEXT("DetailUpkeepText"), ContractsBonusIconPath, 1130.f, 304.f, 230.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("LIFETIME REVENUE"), TEXT("DetailRevenueText"), ContractsRevenueIconPath, 610.f, 388.f, 260.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("LIFETIME COST"), TEXT("DetailCostText"), ContractsRevenueIconPath, 900.f, 388.f, 230.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("LAST ROYALTY"), TEXT("DetailLastRoyaltyText"), ContractsRoyaltyIconPath, 1160.f, 388.f, 220.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("RECORDS"), TEXT("DetailRecordsText"), ContractsRecordIconPath, 610.f, 474.f, 310.f);
        AddContractsLabelValue(Tree, RootCanvas, TEXT("MOMENTUM"), TEXT("DetailMomentumText"), ContractsRevenueIconPath, 960.f, 474.f, 250.f);
        AddCanvasText(Tree, RootCanvas, TEXT("TermProgressLabel"), TEXT("CONTRACT TERM"), 13, MutedText, FMargin(610.f, 572.f, 180.f, 24.f), false);
        UProgressBar* DetailTermProgressBar = ConstructWidget<UProgressBar>(Tree, TEXT("DetailTermProgressBar"));
        DetailTermProgressBar->SetPercent(0.f);
        AddToCanvas(RootCanvas, DetailTermProgressBar, FAnchors(0.f, 0.f), FMargin(790.f, 578.f, 300.f, 12.f));
        AddCanvasText(Tree, RootCanvas, TEXT("DetailProgressText"), TEXT("--"), 15, TextIvory, FMargin(1110.f, 566.f, 210.f, 28.f));
        AddCanvasText(Tree, RootCanvas, TEXT("ProductionProgressLabel"), TEXT("PRODUCTION"), 13, MutedText, FMargin(610.f, 612.f, 180.f, 24.f), false);
        UProgressBar* DetailProductionProgressBar = ConstructWidget<UProgressBar>(Tree, TEXT("DetailProductionProgressBar"));
        DetailProductionProgressBar->SetPercent(0.f);
        AddToCanvas(RootCanvas, DetailProductionProgressBar, FAnchors(0.f, 0.f), FMargin(790.f, 618.f, 300.f, 12.f));
        AddContractsPanelBorder(Tree, RootCanvas, TEXT("BottomActionSurface"), FMargin(34.f, 668.f, 1412.f, 62.f), FLinearColor(0.06f, 0.047f, 0.030f, 0.94f));
        UButton* ViewArtistButton = ConstructWidget<UButton>(Tree, TEXT("ViewArtistButton"));
        ViewArtistButton->SetBackgroundColor(FLinearColor(0.15f, 0.105f, 0.035f, 1.f));
        AddToCanvas(RootCanvas, ViewArtistButton, FAnchors(0.f, 0.f), FMargin(970.f, 680.f, 210.f, 38.f));
        UTextBlock* ViewText = MakeText(Tree, TEXT("ViewArtistButtonText"), TEXT("VIEW ARTIST"), 16, Gold);
        ViewText->SetJustification(ETextJustify::Center);
        ViewArtistButton->AddChild(ViewText);
        UButton* CloseActionButton = ConstructWidget<UButton>(Tree, TEXT("CloseActionButton"), false);
        CloseActionButton->SetBackgroundColor(FLinearColor(0.05f, 0.043f, 0.035f, 1.f));
        AddToCanvas(RootCanvas, CloseActionButton, FAnchors(0.f, 0.f), FMargin(1202.f, 680.f, 210.f, 38.f));
        UTextBlock* CloseText = MakeText(Tree, TEXT("CloseActionButtonText"), TEXT("CLOSE"), 16, TextIvory);
        CloseText->SetJustification(ETextJustify::Center);
        CloseActionButton->AddChild(CloseText);
        RegisterWidgetVariableGuids(WidgetBlueprint);
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
        SetBlueprintSubclassDefault(WidgetBlueprint, TEXT("ItemClass"), ItemBlueprint->GeneratedClass);
        SaveAndCompileWidgetBlueprint(WidgetBlueprint, TEXT("RebuildActiveContractsPanelBlueprint"));
        return true;
    }

#endif

bool UMusicManagerWidgetBlueprintTools::RebuildTopStatusBarBlueprint()
{
#if WITH_EDITOR
    UWidgetBlueprint* WidgetBlueprint = LoadObject<UWidgetBlueprint>(nullptr, TopStatusBarBlueprintPath);
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildTopStatusBarBlueprint: Could not load %s."), TopStatusBarBlueprintPath);
        return false;
    }

    UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
    if (!Tree)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildTopStatusBarBlueprint: WidgetTree is missing."));
        return false;
    }

    WidgetBlueprint->Modify();
    Tree->Modify();

    Tree->RootWidget = nullptr;

    UCanvasPanel* RootCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("RootCanvas"));
    Tree->RootWidget = RootCanvas;

    UBorder* StatusBarRoot = ConstructWidget<UBorder>(Tree, TEXT("StatusBarRoot"));
    StatusBarRoot->SetBrushColor(FLinearColor(0.006f, 0.007f, 0.006f, 1.f));
    StatusBarRoot->SetPadding(FMargin(0.f));
    AddToCanvas(RootCanvas, StatusBarRoot, FAnchors(0.f, 0.f, 1.f, 0.f), FMargin(0.f, 0.f, 0.f, 72.f));

    UCanvasPanel* StatusCanvas = ConstructWidget<UCanvasPanel>(Tree, TEXT("TopStatusCanvas"), false);
    StatusBarRoot->SetContent(StatusCanvas);

    UImage* BackgroundImage = ConstructWidget<UImage>(Tree, TEXT("BackgroundImage"));
    ConfigureImage(BackgroundImage, TopStatusBarSurfacePath, FVector2D(2048.f, 246.f));
    AddToCanvas(StatusCanvas, BackgroundImage, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));

    UHorizontalBox* MainRow = ConstructWidget<UHorizontalBox>(Tree, TEXT("TopStatusMainRow"), false);
    AddToCanvas(StatusCanvas, MainRow, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(24.f, 0.f, 24.f, 0.f));

    USizeBox* BrandSize = ConstructWidget<USizeBox>(Tree, TEXT("BrandGroupSize"), false);
    BrandSize->SetWidthOverride(410.f);
    BrandSize->SetHeightOverride(58.f);
    UHorizontalBox* BrandRow = ConstructWidget<UHorizontalBox>(Tree, TEXT("BrandGroupRow"), false);
    BrandSize->AddChild(BrandRow);
    UImage* BrandIcon = MakeIcon(Tree, TEXT("BrandIconImage"), BrandIconPath, 58.f);
    UTextBlock* BrandText = MakeText(Tree, TEXT("BrandText"), TEXT("MUSICMANAGER"), 18, Gold);
    AddToRow(BrandRow, BrandIcon, FMargin(0.f, 0.f, 14.f, 0.f));
    AddToRow(BrandRow, BrandText, FMargin(0.f));
    AddToRow(MainRow, BrandSize, FMargin(0.f, 0.f, 18.f, 0.f));

    USpacer* FlexibleLeftSpacer = ConstructWidget<USpacer>(Tree, TEXT("TopStatusFlexibleSpacer"), false);
    FlexibleLeftSpacer->SetSize(FVector2D(1.f, 1.f));
    AddToRow(MainRow, FlexibleLeftSpacer, FMargin(0.f), ESlateSizeRule::Fill, 1.f);

    UImage* DateIcon = MakeIcon(Tree, TEXT("DateIconImage"), DateIconPath, 32.f);
    UTextBlock* DateText = MakeText(Tree, TEXT("DateText"), TEXT("January 1955"), 16, TextIvory);
    AddToRow(MainRow, MakePill(Tree, TEXT("DatePill"), 245.f, DateIcon, DateText), FMargin(0.f, 0.f, 14.f, 0.f));

    UImage* LabelIcon = MakeIcon(Tree, TEXT("LabelIconImage"), LabelIconPath, 32.f);
    UTextBlock* LabelNameText = MakeText(Tree, TEXT("LabelNameText"), TEXT("Black Star Records"), 16, TextIvory);
    AddToRow(MainRow, MakePill(Tree, TEXT("LabelPill"), 320.f, LabelIcon, LabelNameText), FMargin(0.f, 0.f, 14.f, 0.f));

    UImage* CashIcon = MakeIcon(Tree, TEXT("CashIconImage"), CashIconPath, 32.f);
    UTextBlock* CashText = MakeText(Tree, TEXT("CashText"), TEXT("Cash $24,500"), 16, TextIvory);
    AddToRow(MainRow, MakePill(Tree, TEXT("CashPill"), 245.f, CashIcon, CashText), FMargin(0.f, 0.f, 14.f, 0.f));

    UImage* ReputationIcon = MakeIcon(Tree, TEXT("ReputationIconImage"), ReputationIconPath, 32.f);
    UTextBlock* ReputationText = MakeText(Tree, TEXT("ReputationText"), TEXT("Reputation 12"), 16, TextIvory);
    AddToRow(MainRow, MakePill(Tree, TEXT("ReputationPill"), 250.f, ReputationIcon, ReputationText), FMargin(0.f, 0.f, 18.f, 0.f));

    UHorizontalBox* ControlRow = ConstructWidget<UHorizontalBox>(Tree, TEXT("TopStatusControlRow"), false);
    AddToRow(MainRow, ControlRow, FMargin(0.f));

    UButton* PauseButton = ConstructWidget<UButton>(Tree, TEXT("PauseButton"));
    ConfigureButton(PauseButton);
    AddButtonIcon(Tree, PauseButton, TEXT("Pause"), PauseIconPath);
    AddToRow(ControlRow, PauseButton, FMargin(0.f, 0.f, 8.f, 0.f));

    UButton* PlayButton = ConstructWidget<UButton>(Tree, TEXT("PlayButton"));
    ConfigureButton(PlayButton);
    AddButtonIcon(Tree, PlayButton, TEXT("Play"), PlayIconPath);
    AddToRow(ControlRow, PlayButton, FMargin(0.f, 0.f, 8.f, 0.f));

    UButton* FastForwardButton = ConstructWidget<UButton>(Tree, TEXT("FastForwardButton"));
    ConfigureButton(FastForwardButton);
    AddButtonIcon(Tree, FastForwardButton, TEXT("FastForward"), FastForwardIconPath);
    AddToRow(ControlRow, FastForwardButton, FMargin(0.f, 0.f, 8.f, 0.f));

    UButton* MenuButton = ConstructWidget<UButton>(Tree, TEXT("MenuButton"));
    ConfigureButton(MenuButton);
    AddButtonIcon(Tree, MenuButton, TEXT("Menu"), MenuIconPath);
    AddToRow(ControlRow, MenuButton, FMargin(0.f));

    FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
    UPackage* Package = WidgetBlueprint->GetOutermost();
    Package->MarkPackageDirty();
    const FString PackageFilename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Package, WidgetBlueprint, *PackageFilename, FSavePackageArgs());
    UE_LOG(LogTemp, Log, TEXT("RebuildTopStatusBarBlueprint: Rebuilt %s Saved=%s."), TopStatusBarBlueprintPath, bSaved ? TEXT("true") : TEXT("false"));
    return bSaved;
#else
    return false;
#endif
}

bool UMusicManagerWidgetBlueprintTools::RebuildNewsFeedBlueprints()
{
#if WITH_EDITOR
    const bool bItemRebuilt = RebuildNewsFeedItemBlueprint();
    if (!bItemRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildNewsFeedBlueprints: Item Blueprint rebuild failed."));
        return false;
    }

    const bool bListRebuilt = RebuildNewsFeedListBlueprint();
    if (!bListRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildNewsFeedBlueprints: List Blueprint rebuild failed."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("RebuildNewsFeedBlueprints: Rebuilt news feed list and item Blueprints."));
    return true;
#else
    return false;
#endif
}

bool UMusicManagerWidgetBlueprintTools::RebuildBottomCommandDockBlueprints()
{
#if WITH_EDITOR
    const bool bItemRebuilt = RebuildCommandItemBlueprint();
    if (!bItemRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildBottomCommandDockBlueprints: Command item Blueprint rebuild failed."));
        return false;
    }

    const bool bPanelRebuilt = RebuildCommandPanelBlueprint();
    if (!bPanelRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildBottomCommandDockBlueprints: Command panel Blueprint rebuild failed."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("RebuildBottomCommandDockBlueprints: Rebuilt bottom command dock Blueprints."));
    return true;
#else
    return false;
#endif
}

bool UMusicManagerWidgetBlueprintTools::RebuildStudioRecordingBlueprints()
{
#if WITH_EDITOR
    const bool bAvailableItemRebuilt = RebuildRecordSongListItemBlueprint(
        RecordSongListBlueprintPath,
        RecordSongListBlueprintPackagePath,
        TEXT("RecordSongListBP"),
        false);
    if (!bAvailableItemRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildStudioRecordingBlueprints: Failed to rebuild available song item Blueprint."));
        return false;
    }

    const bool bSelectedItemRebuilt = RebuildRecordSongListItemBlueprint(
        RecordingRecordListBlueprintPath,
        RecordingRecordListBlueprintPackagePath,
        TEXT("RecordingRecordListBP"),
        true);
    if (!bSelectedItemRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildStudioRecordingBlueprints: Failed to rebuild selected song item Blueprint."));
        return false;
    }

    const bool bPanelRebuilt = RebuildStudioRecordingBlueprint();
    if (!bPanelRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildStudioRecordingBlueprints: Failed to rebuild recording panel Blueprint."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("RebuildStudioRecordingBlueprints: Rebuilt studio recording Blueprints."));
    return true;
#else
    return false;
#endif
}


bool UMusicManagerWidgetBlueprintTools::RebuildActiveContractsBlueprints()
{
#if WITH_EDITOR
    const bool bItemRebuilt = RebuildActiveContractItemBlueprint();
    if (!bItemRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildActiveContractsBlueprints: Active contract item Blueprint rebuild failed."));
        return false;
    }

    const bool bPanelRebuilt = RebuildActiveContractsPanelBlueprint();
    if (!bPanelRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildActiveContractsBlueprints: Active contracts panel Blueprint rebuild failed."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("RebuildActiveContractsBlueprints: Rebuilt active contracts Blueprints."));
    return true;
#else
    return false;
#endif
}
bool UMusicManagerWidgetBlueprintTools::RebuildArtistAuditionPanelBlueprint()
{
#if WITH_EDITOR
    const bool bRebuilt = RebuildAuditionPanelBlueprint();
    if (!bRebuilt)
    {
        UE_LOG(LogTemp, Error, TEXT("RebuildArtistAuditionPanelBlueprint: Audition Blueprint rebuild failed."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("RebuildArtistAuditionPanelBlueprint: Rebuilt artist audition left panel."));
    return true;
#else
    return false;
#endif
}
