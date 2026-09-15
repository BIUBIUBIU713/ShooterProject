// Fill out your copyright notice in the Description page of Project Settings.


#include "Gun.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// Sets default values
AGun::AGun()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneRoot);
	
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(SceneRoot);
	
	MuzzleFlashParticleSystem = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MuzzleFlashParticleSystem"));
	MuzzleFlashParticleSystem->SetupAttachment(SkeletalMesh);

}

// Called when the game starts or when spawned
void AGun::BeginPlay()
{
	//运行时再次保护配置，确保容量至少为1
	MaxEnergy = FMath::Max(MaxEnergy, 1);
	
	//确保充能时间有效，并且大于0
	RechargeDuration = FMath::IsFinite(RechargeDuration) 
		? FMath::Max(RechargeDuration, 0.1f)
		: 2.5f;
	
	//每把枪出生时都是从满能量开始
	CurrentEnergy = MaxEnergy;
	bIsRecharging = false;
	
	Super::BeginPlay();
	
	MuzzleFlashParticleSystem->Deactivate();
	
	if (bUsesEnergy)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s energy initialized: %d / %d, RechargeDuration = %.2f"),
			*GetName(),
			CurrentEnergy,
			MaxEnergy,
			RechargeDuration
		);
	}
}

// Called every frame
void AGun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AGun::PullTrigger()
{
	//没有有效控制器或世界时，不能执行射击
	if (!IsValid(OwnerController) || !IsValid(GetWorld()))
	{
		return false;
	}
	
	if (bUsesEnergy)
	{
		//充能期间或能量耗尽时拒绝开火
		//此处不再自动启动充能
		if (bIsRecharging || CurrentEnergy <= 0)
		{
			return false;
		}
		
		--CurrentEnergy;
		
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s fired: energy %d/%d"),
			*GetName(),
			CurrentEnergy,
			MaxEnergy
		);
	}
	
	MuzzleFlashParticleSystem->Activate(true);
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShootSound, GetActorLocation());
	
	if (OwnerController)
	{
		FVector ViewPointLocation;
		FRotator ViewPointRotation;
		OwnerController->GetPlayerViewPoint(ViewPointLocation, ViewPointRotation);
	
		//控制器原本瞄准的方向
		const FVector AimDirection = ViewPointRotation.Vector();
		
		//防止运行时传入非法数值
		const float SafeHalfAngleDegrees = 
			FMath::IsFinite(SpreadHalfAngleDegrees) ? FMath::Clamp(SpreadHalfAngleDegrees, 0.0f, 45.0f) : 0.0f;
		
		FVector ShotDirection = AimDirection;
		
		if (SafeHalfAngleDegrees > 0.0f)
		{
			//VRandCone使用弧度，需要先将角度转换为弧度
			const float HalfAngleRadians =
				FMath::DegreesToRadians(SafeHalfAngleDegrees);
			
			//在瞄准方向周围的圆锥范围内，随机选择射击方向
			ShotDirection = FMath::VRandCone(
				AimDirection,
				HalfAngleRadians
			);
		}
		
		const FVector EndLocation = ViewPointLocation + MaxRange * ShotDirection;
		
		FHitResult HitResult;
	
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(GetOwner());
		bool IsHit = GetWorld()->LineTraceSingleByChannel(HitResult, ViewPointLocation, EndLocation, ECC_GameTraceChannel1, Params);
	
		if (IsHit)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactParticleSystem, HitResult.ImpactPoint, HitResult.ImpactPoint.Rotation());
			AActor* HitActor = HitResult.GetActor();
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
			UGameplayStatics::ApplyDamage(HitActor, BulletDamage, OwnerController, this, UDamageType::StaticClass());
		}
	}
	
	return true;
}

bool AGun::TryStartRecharge()
{
	if (!IsValid(GetWorld()) || !bUsesEnergy || bIsRecharging || CurrentEnergy >= MaxEnergy)
	{
		return false;
	}
	
	bIsRecharging = true;
	
	GetWorldTimerManager().SetTimer(
		RechargeTimerHandle,
		this,
		&AGun::FinishRecharge,
		RechargeDuration,
		false
	);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s recharge started: %.2f seconds"),
		*GetName(),
		RechargeDuration
	);
	
	return true;
}

void AGun::FinishRecharge()
{
	if (!bIsRecharging)
	{
		return;
	}
	
	CurrentEnergy = MaxEnergy;
	bIsRecharging = false;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s recharge finished: %d/%d"),
		*GetName(),
		CurrentEnergy,
		MaxEnergy
	);
}

void AGun::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RechargeTimerHandle);
	bIsRecharging = false;
	
	Super::EndPlay(EndPlayReason);
}


