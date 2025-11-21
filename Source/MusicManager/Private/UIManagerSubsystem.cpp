// File: Private/UIManagerSubsystem.cpp
#include "UIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Layout.h"
#include "ArtistManagerSubsystem.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIManagerSubsystem, Log, All);

void UUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UArtistManagerSubsystem* Artist = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        Artist->OnArtistSigned.AddDynamic(this, &UUIManagerSubsystem::HandleArtistSigned);
    }
}

void UUIManagerSubsystem::Deinitialize()
{
    if (UArtistManagerSubsystem* Artist = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        Artist->OnArtistSigned.RemoveDynamic(this, &UUIManagerSubsystem::HandleArtistSigned);
    }

    Super::Deinitialize();
}

void UUIManagerSubsystem::RegisterLayout(ULayout* InLayout)
{
    ActiveLayout = InLayout;

    if (InLayout && (!LayoutClass || LayoutClass->HasAnyClassFlags(CLASS_Abstract)))
    {
        LayoutClass = InLayout->GetClass();
    }
}

void UUIManagerSubsystem::UnregisterLayout(ULayout* Layout)
{
    if (ActiveLayout.Get() == Layout)
    {
        ActiveLayout.Reset();
    }
}

void UUIManagerSubsystem::ShowAudition(const FAuditionEvent& EventData)
{
    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->ShowAuditionWidgetWithData(EventData);
    }
}

void UUIManagerSubsystem::HandleNewsCardSelected(const FMusicNewsEvent& EventData)
{
    ExecuteOnGameThread([this, EventData]()
    {
        OnNewsSelected.Broadcast(EventData);
    });
}

void UUIManagerSubsystem::HandleArtistSigned(const FArtistContract& Contract)
{
    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->ShowContract(Contract);
    }
}

void UUIManagerSubsystem::RebuildUI()
{
    ExecuteOnGameThread([this]()
    {
        UGameInstance* GameInstance = GetGameInstance();
        if (!GameInstance)
        {
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("RebuildUI aborted: no GameInstance available."));
            return;
        }

        if (ULayout* ExistingLayout = ActiveLayout.Get())
        {
            ExistingLayout->RemoveFromParent();
            ActiveLayout.Reset();
        }

        TSubclassOf<ULayout> ClassToUse = LayoutClass;
        if (!ClassToUse || ClassToUse->HasAnyClassFlags(CLASS_Abstract))
        {
            UClass* DefaultClass = ULayout::StaticClass();
            if (DefaultClass && !DefaultClass->HasAnyClassFlags(CLASS_Abstract))
            {
                ClassToUse = DefaultClass;
            }
        }

        if (!ClassToUse || ClassToUse->HasAnyClassFlags(CLASS_Abstract))
        {
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("RebuildUI failed: No valid layout class available."));
            return;
        }

        ULayout* NewLayout = CreateWidget<ULayout>(GameInstance, ClassToUse);
        if (!NewLayout)
        {
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("RebuildUI failed: Could not create layout instance."));
            return;
        }

        ActiveLayout = NewLayout;
        LayoutClass = ClassToUse;
        NewLayout->AddToViewport();
    });
}

