// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CookerSettings.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Gun.h"

#include "ShooterSamProjectCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class ESupplyPurchaseResult : uint8
{
	Success UMETA(DisplayName="Success"),
	InvalidPlayer UMETA(DisplayName="Invalid player"),
	InvalidOffer UMETA(DisplayName="Invalid offer"),
	ResourceFull UMETA(DisplayName="Resource full"),
	InsufficientCoins UMETA(DisplayName="Insufficient Coins")
};

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AShooterSamProjectCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;
	
	UPROPERTY(EditAnywhere, CateGory = "Input")
	UInputAction* ShootAction;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AGun> GunClass;
	
	UPROPERTY(VisibleAnywhere)
	AGun* GunMember;
	
	//角色生命上限，由蓝图配置
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Player|Health",
		meta = (ClampMin = "1.0")
	)
	float MaxHealth = 100.0f;
	
	//运行时生命值
	UPROPERTY(
		VisibleInstanceOnly,
		Category = "Player|Health"
	)
	float Health = 0.0f;
	
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintReadOnly)
	bool IsAlive = true;

	/** Constructor */
	AShooterSamProjectCharacter();	
	
	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* InteractAction = nullptr;
	
	//查找范围内最近且没有被遮挡的交互目标
	AActor* FindInteractionTarget();
	
	//按下交互键时执行
	void Interact();
	
	//当前向玩家展示的目标
	TWeakObjectPtr<AActor> CurrentInteractionTarget;
	
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	
	//手动充能输入
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* RechargeAction = nullptr;
	
	//备用电池携带上限
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Player|Battery",
		meta = (ClampMin = "0")
	)
	int32 MaxBatteryCount = 10;
	
	//当前备用电池数量
	UPROPERTY(VisibleInstanceOnly, Category = "Player|Battery")
	int32 CurrentBatteryCount = 0;
	
	//出生金币，便于配置和测试
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Player|Currency",
		meta = (ClampMin = "0")
	)
	int32 StartingCoins = 0;
	
	//当前金币余额
	UPROPERTY(
		VisibleInstanceOnly,
		Category = "Player|Currency"
	)
	int32 Coins = 0;
	
	//保存本角色应用增益前的移动速度
	float BaseWalkSpeed = 0.0f;
	
	//按下充能键时执行
	void Recharge();

public:
	UFUNCTION(BlueprintCallable, Category = "Player|Weapon")
	AGun* GetEquippedGun() const
	{
		return GunMember;
	}
	
	//返回实际增加的金币数量
	UFUNCTION(BlueprintCallable, Category = "Player|Currency")
	int32 AddCoins(int32 Amount);
	
	//扣款成功返回true，失败时余额不变
	UFUNCTION(BlueprintCallable, Category = "Player|Currency")
	bool TrySpendCoins(int32 Cost);
	
	UFUNCTION(BlueprintPure, Category = "Player|Currency")
	int32 GetCoins() const
	{
		return Coins;
	}

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();
	
	UFUNCTION(BlueprintCallable, Category="Player|Health")
	float RestoreHealth(float Amount);
	
	UFUNCTION(BlueprintCallable, Category="Player|Health")
	float GetCurrentHealth() const
	{
		return Health;
	}
	
	UFUNCTION(BlueprintPure, Category="Player|Health")
	float GetMaxHealth() const
	{
		return MaxHealth;
	}
	
	UFUNCTION(BlueprintPure, Category="Player|Health")
	float GetHealthPercent() const
	{
		return MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;
	}
	
	//增加备用电池，返回实际增加的数量
	UFUNCTION(BlueprintCallable, Category = "Player|Battery")
	int32 AddBatteries(int32 Amount);
	
	UFUNCTION(BlueprintPure, Category="Player|Battery")
	int32 GetCurrentBatteryCount() const
	{
		return CurrentBatteryCount;
	}
	
	UFUNCTION(BlueprintPure, Category="Player|Battery")
	int32 GetMaxBatteryCount() const
	{
		return MaxBatteryCount;
	}
	
	//按固定价格购买电池，需多补充Amount
	UFUNCTION(BlueprintCallable, Category = "Player|Supply")
	ESupplyPurchaseResult TryBuyBatteries(int32 Amount, int32 Cost);
	
	//按固定价格购买治疗，最多恢复Amount点生命
	UFUNCTION(BlueprintCallable, Category = "Player|Supply")
	ESupplyPurchaseResult TryBuyHealing(float Amount, int32 Cost);
	
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
	UFUNCTION()
	void OnDamageTaken(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	//根据本局增益等级重新计算角色属性
	UFUNCTION(BlueprintCallable, Category = "Player|Upgrades")
	void RefreshUpgradeEffects();
	
	void Shoot();
	void UpdateHUD();
};

