#include "SupplyStation.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ShooterSamProjectPlayerController.h"

ASupplyStation::ASupplyStation()
{
    PrimaryActorTick.bCanEverTick = false;

    InteractionSphere =
        CreateDefaultSubobject<USphereComponent>(
            TEXT("InteractionSphere"));

    SetRootComponent(InteractionSphere);

    InteractionSphere->InitSphereRadius(180.0f);
    InteractionSphere->SetGenerateOverlapEvents(true);
    InteractionSphere->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly);

    InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(
        ECC_Pawn, ECR_Overlap);

    StationMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("StationMesh"));

    StationMesh->SetupAttachment(InteractionSphere);
    StationMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StationMesh->SetGenerateOverlapEvents(false);
}

bool ASupplyStation::CanInteract(
    AShooterSamProjectCharacter* Player) const
{
    if (!IsValid(Player) || !Player->IsAlive ||
        !Player->IsPlayerControlled() || !IsValid(GetWorld()))
    {
        return false;
    }

    if (!InteractionSphere->IsOverlappingComponent(
        Player->GetCapsuleComponent()))
    {
        return false;
    }

    // 打开菜单和确认购买时，都不能隔墙操作
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(Player);

    TArray<AActor*> AttachedActors;
    Player->GetAttachedActors(AttachedActors);
    Params.AddIgnoredActors(AttachedActors);

    const bool bBlocked = GetWorld()->LineTraceTestByChannel(
        Player->GetPawnViewLocation(),
        GetActorLocation(),
        ECC_Visibility,
        Params
    );

    return !bBlocked;
}

FText ASupplyStation::GetInteractionText() const
{
    return NSLOCTEXT(
        "SupplyStation", "Interact", "[E] Open supply station");
}

bool ASupplyStation::TryInteract(
    AShooterSamProjectCharacter* Player)
{
    if (!CanInteract(Player))
    {
        return false;
    }

    AShooterSamProjectPlayerController* PC =
        Cast<AShooterSamProjectPlayerController>(
            Player->GetController());

    return IsValid(PC) && PC->OpenSupplyMenu(this);
}

ESupplyPurchaseResult ASupplyStation::TryPurchase(
    AShooterSamProjectCharacter* Player,
    ESupplyProduct Product)
{
    if (!CanInteract(Player))
    {
        return ESupplyPurchaseResult::InvalidPlayer;
    }

    AShooterSamProjectPlayerController* PC =
        Cast<AShooterSamProjectPlayerController>(
            Player->GetController());

    // 必须正在使用这一个补给站的菜单
    if (!IsValid(PC) || PC->ActiveSupplyStation != this)
    {
        return ESupplyPurchaseResult::InvalidPlayer;
    }

    ESupplyPurchaseResult Result =
        ESupplyPurchaseResult::InvalidOffer;

    switch (Product)
    {
    case ESupplyProduct::Battery:
        Result = Player->TryBuyBatteries(
            BatteryAmount, BatteryPrice);
        break;

    case ESupplyProduct::Healing:
        Result = Player->TryBuyHealing(
            HealingAmount, HealingPrice);
        break;

    default:
        break;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("%s purchase: player=%s, product=%d, result=%d"),
        *GetName(),
        *GetNameSafe(Player),
        static_cast<int32>(Product),
        static_cast<int32>(Result)
    );

    return Result;
}

FText ASupplyStation::GetOfferText(
    AShooterSamProjectCharacter* Player,
    ESupplyProduct Product) const
{
    if (!IsValid(Player))
    {
        return FText::FromString(TEXT("Player unavailable"));
    }

    if (Product == ESupplyProduct::Battery)
    {
        const int32 Space = FMath::Max(
            Player->GetMaxBatteryCount() -
            Player->GetCurrentBatteryCount(), 0);

        const int32 ActualAmount =
            FMath::Min(FMath::Max(BatteryAmount, 0), Space);

        return FText::FromString(FString::Printf(
            TEXT("Battery +%d | Cost: %d | Coins: %d"),
            ActualAmount,
            BatteryPrice,
            Player->GetCoins()
        ));
    }

    if (Product == ESupplyProduct::Healing)
    {
        if (!FMath::IsFinite(HealingAmount) || HealingAmount <= 0.0f)
        {
            return FText::FromString(TEXT("Invalid healing offer"));
        }

        const float Missing = FMath::Max(
            Player->GetMaxHealth() -
            Player->GetCurrentHealth(), 0.0f);

        const float ActualAmount =
            FMath::Min(HealingAmount, Missing);

        return FText::FromString(FString::Printf(
            TEXT("Health +%.1f | Cost: %d | Coins: %d"),
            ActualAmount,
            HealingPrice,
            Player->GetCoins()
        ));
    }

    return FText::FromString(TEXT("Invalid product"));
}