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

void UUIManagerSubsystem::HandleCommandAction(const FString& CommandName)
{
    TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);

    AsyncTask(ENamedThreads::GameThread, [WeakThis, CommandName]()
    {
        UUIManagerSubsystem* Self = WeakThis.Get();
        if (!IsValid(Self))
        {
            return;
        }

        if (CommandName == TEXT("Contracts"))
        {
            UGameInstance* GameInstance = Self->GetGameInstance();
            if (!IsValid(GameInstance))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("HandleCommandAction: GameInstance is invalid."));
                return;
            }

            UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
            if (!IsValid(ArtistSubsystem))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("HandleCommandAction: ArtistManagerSubsystem is unavailable."));
                return;
            }

            if (ArtistSubsystem->ActiveContracts.Num() <= 0)
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("HandleCommandAction: No active contracts to display."));
                return;
            }

            const FArtistContract& Contract = ArtistSubsystem->ActiveContracts[0];

            ULayout* Layout = Self->ActiveLayout.Get();
            if (!IsValid(Layout))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("HandleCommandAction: No active layout registered to show contracts."));
                return;
            }

            Layout->ShowContract(Contract);
        }
    });
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

