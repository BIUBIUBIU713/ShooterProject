// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterRangedEnemy.h"

#include "Gun.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

void AShooterRangedEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	if (!GunClass)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: GunClass is not configured"),
			*GetName()
		);
		return;
	}
	
	if (!GetMesh()->DoesSocketExist(WeaponSocketName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: Weapon socket '%s' was not found"),
			*GetName(),
			*WeaponSocketName.ToString()
		);
		
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	EquippedGun = GetWorld()->SpawnActor<AGun>(
		GunClass,
		GetActorLocation(),
		GetActorRotation(),
		SpawnParams
	);
	
	if (!IsValid(EquippedGun.Get()))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s: Failed to spawn gun"),
			*GetName()
		);
		
		return;
	}
	
	//枪是附属物，不参与物理碰撞
	EquippedGun->SetActorEnableCollision(false);
	
	EquippedGun->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponSocketName
	);
	
	EquippedGun->OwnerController = GetController();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s equipped gun %s"),
		*GetName(),
		*GetNameSafe(EquippedGun.Get())
	);
}

bool AShooterRangedEnemy::TryFire()
{
	//当前死亡状态与生命值都检查，避免零血量时继续开枪
	if (IsDead() || GetCurrentHealth() <= 0.0f)
	{
		return false;
	}
	
	if (!IsValid(EquippedGun.Get()) || !IsValid(GetController()))
	{
		return false;
	}
	
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}
	
	const double CurrentTime = World->GetTimeSeconds();
	
	if (CurrentTime < NextAllowedFireTime)
	{
		return false;
	}
	
	//开枪前刷新，避免只在BeginPlay获取控制器造成时序问题
	EquippedGun->OwnerController = GetController();
	if (!EquippedGun->PullTrigger())
	{
		return false;
	}
	
	NextAllowedFireTime = CurrentTime + FMath::Max(FireInterval, 0.01f);
	
	return true;
}

void AShooterRangedEnemy::EndPlay(
	const EEndPlayReason::Type EndPlayReason	
)
{
	if (IsValid(EquippedGun.Get()))
	{
		EquippedGun->Destroy();
		EquippedGun = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}
