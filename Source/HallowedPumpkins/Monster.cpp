// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Particles/ParticleEmitter.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HallowedPumpkinsCharacter.h"
#include "AIController.h"
#include "Net/UnrealNetwork.h"
#include "AmmoBox.h"
#include "HPGameState.h"
#include "HPPlayerState.h"

// Sets default values
AMonster::AMonster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 42.f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 380.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	HiddenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HiddenMesh"));
	HiddenMesh->SetupAttachment(RootComponent);

	hidden = true;
	Speed = 600.f;
	HiddenSpeed = 50.f;

	Dead = false;

	AttackRange = 200.f;

	TransformCDTimer = 0.f;
	AttackCDTimer = 0.f;
	ScheduleInteractTimer = 0.f;
	BehaviourLogicCD = 0.5f;
	ScreamCDTimer = 0.f;

	FixatedTarget = nullptr;

	ScreamSoundID = 0;
}

void AMonster::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMonster::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMonster::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMonster::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMonster::LookUpAtRate);

	PlayerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &AMonster::ZoomIn);
	PlayerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &AMonster::ZoomOut);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMonster::Interact);

	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AMonster::Attack);
}

void AMonster::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMonster::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMonster::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMonster::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

// Called when the game starts or when spawned
void AMonster::BeginPlay()
{
	Super::BeginPlay();
	
	GetCharacterMovement()->MaxWalkSpeed = HiddenSpeed;
	GetMesh()->SetSkeletalMesh(nullptr, false);
	GetCapsuleComponent()->SetCapsuleHalfHeight(42.f);

	if (HasAuthority())
		ScreamSoundID = FMath::RandRange(0, 5);
}

void AMonster::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!HasAuthority())
		return;

	if (NewController->IsPlayerController())
	{
		TArray<AActor*> ammoboxes;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmmoBox::StaticClass(), ammoboxes);

		for (auto i : ammoboxes)
		{
			AAmmoBox* ammb = Cast<AAmmoBox>(i);
			++ammb->Charges;
		}
	}
}

void AMonster::Restart()
{
	Super::Restart();

	if (IsLocallyControlled())
		if (AHPPlayerState* ps = Cast<AHPPlayerState>(GetPlayerState()))
			ps->RequestMonsterHiddenState();
}

// Called every frame
void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Dead)
		return;

	if (TransformCDTimer > 0.f)
	{
		if (TransformCDTimer > DeltaTime)
			TransformCDTimer -= DeltaTime;
		else
			TransformCDTimer = 0.f;
	}

	if (!HasAuthority())
		return;

	if (AttackCDTimer > 0.f)
	{
		if (AttackCDTimer > DeltaTime)
			AttackCDTimer -= DeltaTime;
		else
			AttackCDTimer = 0.f;
	}

	if (ScheduleInteractTimer > 0.f)
	{
		if (ScheduleInteractTimer > DeltaTime)
			ScheduleInteractTimer -= DeltaTime;
		else
		{
			Interact();
			ScheduleInteractTimer = 0.f;
		}
	}

	if (!FixatedTarget)
	{
		if (BehaviourLogicCD > 0.f)
		{
			if (BehaviourLogicCD > DeltaTime)
				BehaviourLogicCD -= DeltaTime;
			else
			{
				AIBehaviourLogic(DeltaTime);
				BehaviourLogicCD = 0.5f;
			}
		}
	}
	else AIBehaviourLogic(DeltaTime);

	if (ScreamCDTimer <= 0.f)
	{
		if (!hidden)
		{
			if (AHallowedPumpkinsCharacter* nearhuman = FindNearestHuman())
			{
				if (GetDistanceTo(nearhuman) <= (AttackRange * 10.f))
				{
					SendScreamSound();
					ScreamCDTimer = 60.f;
				}
			}
		}
	}
	else ScreamCDTimer -= DeltaTime;
}

void AMonster::ZoomIn()
{
	if (CameraBoom->TargetArmLength > 100.f)
		CameraBoom->TargetArmLength -= 25.f;
}

void AMonster::ZoomOut()
{
	if (CameraBoom->TargetArmLength < 400.f)
		CameraBoom->TargetArmLength += 25.f;
}

void AMonster::Interact()
{
	if (!HasAuthority())
	{
		if (TransformCDTimer != 0.f)
			return;

		TransformCDTimer = 5.f;
	}

	RequestInteract();
}

void AMonster::RequestInteract_Implementation()
{
	if (TransformCDTimer != 0.f)
		return;

	TransformCDTimer = 5.f;

	if (!hidden)
		SendInteract(true);
	else SendInteract(false);
}

void AMonster::SendInteract_Implementation(bool phide)
{
	if (phide)
	{
		GetCharacterMovement()->MaxWalkSpeed = HiddenSpeed;

		GetMesh()->SetSkeletalMesh(nullptr, false);
		HiddenMesh->SetStaticMesh(HiddenMeshModel);
		GetCapsuleComponent()->SetCapsuleHalfHeight(42.f);

		hidden = true;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = Speed;

		GetMesh()->SetSkeletalMesh(UnhiddenMeshModel);
		GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);
		HiddenMesh->SetStaticMesh(nullptr);

		hidden = false;
	}
}

void AMonster::Attack()
{
	if(!hidden)
		RequestAttack();
}

void AMonster::RequestAttack_Implementation()
{
	if (hidden)
		return;

	// AI only
	if (FixatedTarget)
	{
		if ((GetDistanceTo(FixatedTarget) < AttackRange) && IsActorFrontOfMe(FixatedTarget))
		{
			FixatedTarget->Die();
			SendAttackAnim();
			return;
		}
	}

	if (AttackCDTimer != 0.f)
		return;

	AttackCDTimer = 1;

	if (AController* cont = GetController())
		if (cont->IsPlayerController())
			SendAttackAnim();

	TArray<AActor*> HumanActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHallowedPumpkinsCharacter::StaticClass(), HumanActors);

	for (int32 i = 0; i < HumanActors.Num();)
	{
		AHallowedPumpkinsCharacter* HumanActor = Cast<AHallowedPumpkinsCharacter>(HumanActors[i]);
		if (HumanActor->Dead || (GetDistanceTo(HumanActor) > AttackRange) || !IsActorFrontOfMe(HumanActor))
			HumanActors.RemoveAt(i);
		else ++i;
	}

	if (!HumanActors.Num())
		return;

	float distance = 0;
	if (AActor* Target = UGameplayStatics::FindNearestActor(GetActorLocation(), HumanActors, distance))
	{
		if (AHallowedPumpkinsCharacter* TargetC = Cast<AHallowedPumpkinsCharacter>(Target))
		{
			TargetC->Die();
			if(AController* cont = GetController())
				if(!cont->IsPlayerController())
					SendAttackAnim();
		}
	}
}

void AMonster::Die_Implementation()
{
	if (hidden)
	{
		GetMesh()->SetSkeletalMesh(UnhiddenMeshModel);
		GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);
		HiddenMesh->SetStaticMesh(nullptr);
	}

	Dead = true;

	if(HasAuthority() && (!GetController()->IsPlayerController()))
		GetController<AAIController>()->StopMovement();

	if ((HasAuthority() && GetController()->IsPlayerController()) || IsLocallyControlled())
	{
		GetCharacterMovement()->DisableMovement();
		DisableInput(GetController<APlayerController>());
	}

	if (HasAuthority())
	{
		TArray<AActor*> ammoboxes;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAmmoBox::StaticClass(), ammoboxes);

		if (ammoboxes.Num())
		{
			int32 randomi = FMath::RandRange(0, ammoboxes.Num() - 1);
			AAmmoBox* ammb = Cast<AAmmoBox>(ammoboxes[randomi]);
			++ammb->Charges;
		}

		AHPGameState* gs = Cast<AHPGameState>(UGameplayStatics::GetGameState(GetWorld()));
		gs->MonsterKilled();
	}
}

TArray<AHallowedPumpkinsCharacter*> AMonster::FindHumansInDistance(float minD, float maxD)
{
	TArray<AActor*> HumanActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHallowedPumpkinsCharacter::StaticClass(), HumanActors);

	TArray<AHallowedPumpkinsCharacter*> Results;

	for (int32 i = 0; i < HumanActors.Num(); ++i)
	{
		AHallowedPumpkinsCharacter* HumanActor = Cast<AHallowedPumpkinsCharacter>(HumanActors[i]);
		float Distance = GetDistanceTo(HumanActor);
		if (!HumanActor->Dead && (Distance <= maxD) && (Distance >= minD))
			Results.Add(Cast<AHallowedPumpkinsCharacter>(HumanActor));
	}

	return Results;
}

AHallowedPumpkinsCharacter* AMonster::FindNearestHuman()
{
	TArray<AActor*> HumanActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHallowedPumpkinsCharacter::StaticClass(), HumanActors);

	for (int32 i = 0; i < HumanActors.Num();)
	{
		AHallowedPumpkinsCharacter* HumanActor = Cast<AHallowedPumpkinsCharacter>(HumanActors[i]);
		if (HumanActor->Dead)
			HumanActors.RemoveAt(i);
		else ++i;
	}

	if (!HumanActors.Num())
		return nullptr;

	float distance = 0;
	if (AActor* Target = UGameplayStatics::FindNearestActor(GetActorLocation(), HumanActors, distance))
	{
		if (AHallowedPumpkinsCharacter* TargetC = Cast<AHallowedPumpkinsCharacter>(Target))
			return TargetC;
	}

	return nullptr;
}

bool AMonster::IsActorFrontOfMe(AActor* OtherActor)
{
	FVector lookVec = GetActorRotation().Vector();
	FVector dirToTarget = OtherActor->GetActorLocation() - GetActorLocation();

	float dotp = FVector::DotProduct(lookVec, dirToTarget);

	if (dotp > 0)
		return true;
	else return false;
}

bool AMonster::AmIFrontOf(AActor* OtherActor)
{
	FVector lookVec = OtherActor->GetActorRotation().Vector();
	FVector dirToTarget = GetActorLocation() - OtherActor->GetActorLocation();

	float dotp = FVector::DotProduct(lookVec, dirToTarget);

	if (dotp > 0)
		return true;
	else return false;
}

void AMonster::AIBehaviourLogic(float DeltaTime)
{
	AAIController* aic = GetController<AAIController>();

	if (!aic)
		return;
	
	if (FixatedTarget)
	{
		if (FixatedTarget->Dead)
		{
			if (AHallowedPumpkinsCharacter* nearhuman = FindNearestHuman())
			{
				if (GetDistanceTo(nearhuman) <= (AttackRange * 10.f))
				{
					FixatedTarget = nearhuman;
					return;
				}
			}

			TransformCDTimer = 0.f;
			ScheduleInteractTimer = 2.f;
			FixatedTarget = nullptr;
			return;
		}

		Attack();

		if (GetVelocity().IsZero())
			aic->MoveToActor(FixatedTarget);

		return;
	}

	if (ScheduleInteractTimer > 0.f)
		return;

	if (!GetVelocity().IsZero())
		return;
	
	AHallowedPumpkinsCharacter* NearestHuman = FindNearestHuman();

	if (!NearestHuman)
		return;

	if (GetDistanceTo(NearestHuman) <= AttackRange)
	{
		FRotator myrot = GetActorRotation();
		myrot.Yaw = (NearestHuman->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
		SetActorRotation(myrot);
		Interact();
		aic->MoveToActor(NearestHuman);
		FixatedTarget = NearestHuman;
		return;
	}

	if (FMath::RandRange(1, 60) == 1)
	{
		if (GetDistanceTo(NearestHuman) <= (AttackRange * 10.f))
		{
			Interact();
			aic->MoveToActor(NearestHuman);
			FixatedTarget = NearestHuman;
			return;
		}
	}

	TArray<AHallowedPumpkinsCharacter*> NearHumans = FindHumansInDistance(0.f, 4000.f);
	if (NearHumans.Num())
	{
		if (FMath::RandRange(1, 20) == 1)
		{
			bool SomeOneHasAmmo = false;
			for (int32 i = 0; i < NearHumans.Num(); ++i)
			{
				if (NearHumans[i]->ammo > 0)
				{
					SomeOneHasAmmo = true;
					break;
				}
			}

			if (!SomeOneHasAmmo)
			{
				AHallowedPumpkinsCharacter* NearestHumanNoAmmo = nullptr;
				float DistanceFromNearestActor = TNumericLimits<float>::Max();

				for (AHallowedPumpkinsCharacter* ActorToCheck : NearHumans)
				{
					if (ActorToCheck)
					{
						const float DistanceFromActorToCheck = (GetActorLocation() - ActorToCheck->GetActorLocation()).SizeSquared();
						if (DistanceFromActorToCheck < DistanceFromNearestActor)
						{
							NearestHumanNoAmmo = ActorToCheck;
							DistanceFromNearestActor = DistanceFromActorToCheck;
						}
					}
				}

				if (NearestHumanNoAmmo)
				{
					Interact();
					aic->MoveToActor(NearestHumanNoAmmo);
					FixatedTarget = NearestHumanNoAmmo;
					return;
				}
			}
		}
	}
	else
	{
		if (FMath::RandRange(1, 1000) ==  1)
		{
			TArray<AHallowedPumpkinsCharacter*> DistantHumans = FindHumansInDistance(4001.f, TNumericLimits<float>::Max());
			bool SomeOneHasAmmo = false;
			for (int32 i = 0; i < DistantHumans.Num(); ++i)
			{
				if (DistantHumans[i]->ammo > 0)
				{
					SomeOneHasAmmo = true;
					break;
				}
			}

			if (!SomeOneHasAmmo)
			{
				AHallowedPumpkinsCharacter* NearestHumanNoAmmo = nullptr;
				float DistanceFromNearestActor = TNumericLimits<float>::Max();

				for (AHallowedPumpkinsCharacter* ActorToCheck : DistantHumans)
				{
					if (ActorToCheck)
					{
						const float DistanceFromActorToCheck = (GetActorLocation() - ActorToCheck->GetActorLocation()).SizeSquared();
						if (DistanceFromActorToCheck < DistanceFromNearestActor)
						{
							NearestHumanNoAmmo = ActorToCheck;
							DistanceFromNearestActor = DistanceFromActorToCheck;
						}
					}
				}

				if (NearestHumanNoAmmo)
				{
					Interact();
					aic->MoveToActor(NearestHumanNoAmmo);
					FixatedTarget = NearestHumanNoAmmo;
					return;
				}
			}
		}
	}

	if (FMath::RandRange(1, 5000) == 1)
	{
		Interact();
		aic->MoveToActor(NearestHuman);
		FixatedTarget = NearestHuman;
		return;
	}
	
	if (FMath::RandRange(1, 10) == 1)
	{
		TArray<AHallowedPumpkinsCharacter*> AllHumans = FindHumansInDistance(0.f, TNumericLimits<float>::Max());

		bool SomeOneSeeingMe = false;
		for (int32 i = 0; i < AllHumans.Num(); ++i)
		{
			if ((AmIFrontOf(AllHumans[i]) && (GetDistanceTo(AllHumans[i]) < 9000.f)) || (GetDistanceTo(AllHumans[i]) <= AttackRange * 10.f))
			{
				SomeOneSeeingMe = true;
				break;
			}
		}

		if (!SomeOneSeeingMe)
		{
			FVector dir = (NearestHuman->GetActorLocation() - GetActorLocation()).GetSafeNormal() * FMath::FRandRange(100.f, 500.f);
			aic->MoveToLocation(GetActorLocation() + dir);
			return;
		}
	}
}

void AMonster::SendScreamSound_Implementation()
{
	ImplementScreamSound();
}

void AMonster::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMonster, ScreamSoundID);
}
