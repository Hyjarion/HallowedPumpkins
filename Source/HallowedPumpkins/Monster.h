// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Monster.generated.h"

class AHallowedPumpkinsCharacter;

UCLASS()
class HALLOWEDPUMPKINS_API AMonster : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* HiddenMesh;

public:
	// Sets default values for this character's properties
	AMonster();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HiddenSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* HiddenMeshModel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* UnhiddenMeshModel;

	UPROPERTY(BluePrintReadOnly)
	bool hidden;

	UPROPERTY(BluePrintReadOnly)
	bool Dead;

	UPROPERTY(BluePrintReadOnly)
	float TransformCDTimer;
	float AttackCDTimer;
	float ScheduleInteractTimer;
	float BehaviourLogicCD;

	UPROPERTY(BluePrintReadWrite, Replicated)
	int ScreamSoundID;

	UFUNCTION(NetMulticast, Reliable)
	void Die();

	UFUNCTION(NetMulticast, Reliable)
	void SendInteract(bool phide);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void Restart() override;

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

	void ZoomIn();
	void ZoomOut();

	void Interact();

	void Attack();

	AHallowedPumpkinsCharacter* FindNearestHuman();
	TArray<AHallowedPumpkinsCharacter*> FindHumansInDistance(float minD, float maxD);
	bool IsActorFrontOfMe(AActor* OtherActor);
	bool AmIFrontOf(AActor* OtherActor);

	AHallowedPumpkinsCharacter* FixatedTarget;
	void AIBehaviourLogic(float DeltaTime);

	// Net functions
	UFUNCTION(Server, Reliable)
	void RequestInteract();


	UFUNCTION(Server, Unreliable)
	void RequestAttack();
	UFUNCTION(BlueprintImplementableEvent)
	void SendAttackAnim();

	float ScreamCDTimer;
	UFUNCTION(NetMulticast, Reliable)
	void SendScreamSound();
	UFUNCTION(BlueprintImplementableEvent)
	void ImplementScreamSound();


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
