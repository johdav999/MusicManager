#include "AuditionEventActor.h"

#include "Async/Async.h"
#include "EventSubsystem.h"
#include "Layout.h"

AAuditionEventActor::AAuditionEventActor()
{
    PrimaryActorTick.bCanEverTick = false;
    ActiveWidget = nullptr;
}

void AAuditionEventActor::StartAudition()
{
    // Always marshal to the game thread before touching UObjects that live on the UI tree.
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<AAuditionEventActor> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (AAuditionEventActor* StrongThis = WeakThis.Get())
            {
                StrongThis->StartAudition();
            }
        });
        return;
    }

    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (GameInstance == nullptr)
    {
        return;
    }

    UEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UEventSubsystem>();
    if (!IsValid(EventSubsystem))
    {
        return;
    }

    ULayout* Layout = EventSubsystem->LayoutWeak.Get();
    if (!IsValid(Layout))
    {
        return;
    }

    // Reuse the audition widget bound inside the layout instead of spawning a new one.
    UAuditionWidget* LayoutAuditionWidget = Layout->GetAuditionWidget();
    if (!IsValid(LayoutAuditionWidget))
    {
        return;
    }

    ActiveWidget = LayoutAuditionWidget;

    // Push the audition data to the widget so the UI stays in sync with this actor.
    ActiveWidget->AuditionData = AuditionData;

    // Ensure the widget is visible before interacting with it.
    if (ActiveWidget->GetVisibility() != ESlateVisibility::Visible)
    {
        ActiveWidget->SetVisibility(ESlateVisibility::Visible);
    }

    // Bind delegates only once to avoid duplicate callbacks when the widget is reused.
    if (!ActiveWidget->OnSignArtist.IsAlreadyBound(this, &AAuditionEventActor::HandleSignArtist))
    {
        ActiveWidget->OnSignArtist.AddDynamic(this, &AAuditionEventActor::HandleSignArtist);
    }

    if (!ActiveWidget->OnPass.IsAlreadyBound(this, &AAuditionEventActor::HandlePassOnArtist))
    {
        ActiveWidget->OnPass.AddDynamic(this, &AAuditionEventActor::HandlePassOnArtist);
    }
}

void AAuditionEventActor::FinalizeDeal(bool bAcceptDeal)
{
    AuditionData.bSignedArtist = bAcceptDeal;
    AuditionData.Outcome = bAcceptDeal ? TEXT("Deal Accepted") : TEXT("Deal Rejected");

    if (ActiveWidget)
    {
        ActiveWidget->AuditionData = AuditionData;
        ActiveWidget->SetVisibility(ESlateVisibility::Hidden);
    }

    OnNegotiationUpdated.Broadcast();
}

void AAuditionEventActor::HandleSignArtist()
{
    FinalizeDeal(true);
}

void AAuditionEventActor::HandlePassOnArtist()
{
    FinalizeDeal(false);
}
