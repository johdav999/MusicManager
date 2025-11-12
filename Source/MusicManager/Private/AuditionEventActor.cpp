#include "AuditionEventActor.h"

AAuditionEventActor::AAuditionEventActor()
{
    PrimaryActorTick.bCanEverTick = false;
    ActiveWidget = nullptr;
}

void AAuditionEventActor::StartAudition()
{
    if (!WidgetClass)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        ActiveWidget = CreateWidget<UAuditionWidget>(World, WidgetClass);
        if (ActiveWidget)
        {
            ActiveWidget->AuditionData = AuditionData;
            ActiveWidget->AddToViewport();
            ActiveWidget->OnSignArtist.AddDynamic(this, &AAuditionEventActor::HandleSignArtist);
            ActiveWidget->OnPass.AddDynamic(this, &AAuditionEventActor::HandlePassOnArtist);
        }
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
