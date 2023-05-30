// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HallowedPumpkinsCharacter.generated.h"

UCLASS(config=Game)
class AHallowedPumpkinsCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	//class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FPScamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* WeaponMesh;

public:
	AHallowedPumpkinsCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float BaseLookUpRate;

	//UPROPERTY()
	bool Sprinting;
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool RefillingWater;

	float StaminaRegenTimer;
	float StaminaLossTimer;
	float ReloadTimer;
	float ShootCDTimer;
	float WaterLossTimer;
	float WaterRegenTimer;

	UPROPERTY(BlueprintReadWrite, Replicated)
	int Stamina;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WalkSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SprintSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int ammo;
	UPROPERTY(BlueprintReadWrite, Replicated)
	int Water;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WaterRange;

	UPROPERTY(BlueprintReadWrite, Replicated)
	bool Watering;

	UPROPERTY(BlueprintReadWrite)
	bool GunEquipped;

	UPROPERTY(BlueprintReadWrite)
	bool JustFired;

	UFUNCTION(BlueprintImplementableEvent)
	void JustFiredBP();
	UFUNCTION(BlueprintImplementableEvent)
	void EquipChanged(bool newvalue);

	UPROPERTY(BlueprintReadWrite)
	bool Aiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<UUserWidget> CrosshairWidget;
	UUserWidget* CrosshairWidget_i;

	UPROPERTY(BlueprintReadWrite, Replicated)
	bool isReloading;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* ShootSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* ReloadSound;

	UPROPERTY(BluePrintReadOnly)
	bool Dead;

	UFUNCTION(NetMulticast, Reliable)
	void Die();

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	void Jump();

	void Sprint();
	void SprintStop();

	void Shoot();
	void ShootReleased();
	void Aim();
	void AimStop();

	void Interact();
	void ChangeWeapon();

	void WaterPumpkin();

	void Tick(float DeltaSeconds);
	void BeginPlay();
	virtual void Restart() override;
	FVector GetPawnViewLocation() const override;

	bool IsActorFrontOfMe(AActor* OtherActor);

	// Net Functions
	UFUNCTION(Server, Unreliable)
	void RequestSprint();
	UFUNCTION(NetMulticast, Reliable)
	void SendSprint();
	UFUNCTION(Server, Unreliable)
	void RequestSprintStop();
	UFUNCTION(NetMulticast, Reliable)
	void SendSprintStop();

	UFUNCTION(Server, Unreliable)
	void RequestChangeWeapon();
	UFUNCTION(NetMulticast, Reliable)
	void SendChangeWeapon(bool newvalue);

	UFUNCTION(Server, Unreliable)
	void RequestAim();
	UFUNCTION(NetMulticast, Reliable)
	void SendAim();
	UFUNCTION(Server, Unreliable)
	void RequestAimStop();
	UFUNCTION(NetMulticast, Reliable)
	void SendAimStop();

	UFUNCTION(Server, Unreliable)
	void RequestShoot();
	UFUNCTION(NetMulticast, Reliable)
	void SendShoot(bool impact, FTransform impactpos, bool pwater = false);
	UFUNCTION(Server, Unreliable)
	void RequestShootStop();
	UFUNCTION(NetMulticast, Reliable)
	void SendShootStop();

	UFUNCTION(Server, Unreliable)
	void RequestInteract();
	UFUNCTION(NetMulticast, Reliable)
	void SendInteract(bool gun);

	UFUNCTION(Server, Reliable)
	void StopRefill();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	//FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFPScamera() const { return FPScamera; }
};

