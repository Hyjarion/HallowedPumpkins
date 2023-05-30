// Copyright Epic Games, Inc. All Rights Reserved.

#include "HallowedPumpkinsCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"
#include "AmmoBox.h"
#include "WaterBucket.h"
#include "Monster.h"
#include "Pumpkin.h"
#include "Net/UnrealNetwork.h"
#include "HPGameState.h"
#include "HPPlayerState.h"

//////////////////////////////////////////////////////////////////////////
// AHallowedPumpkinsCharacter

AHallowedPumpkinsCharacter::AHallowedPumpkinsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	WalkSpeed = 200.f;
	SprintSpeed = 600.f;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 300.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	//CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	//CameraBoom->SetupAttachment(RootComponent);
	//CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	//CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FPScamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPScamera"));
	FPScamera->SetupAttachment(GetMesh(), FName(TEXT("head")));
	FPScamera->bUsePawnControlRotation = true;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), FName(TEXT("middle_01_r")));

	Sprinting = false;
	Watering = false;
	RefillingWater = false;

	StaminaRegenTimer = 0.f;
	StaminaLossTimer = 0.f;
	ReloadTimer = 0.f;
	ShootCDTimer = 0.f;
	WaterLossTimer = 0.f;
	WaterRegenTimer = 0.f;

	Stamina = 100;

	JustFired = false;
	Aiming = false;

	ammo = 2;
	isReloading = false;

	Water = 100;
	WaterRange = 200.f;
	GunEquipped = false;

	Dead = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHallowedPumpkinsCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AHallowedPumpkinsCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AHallowedPumpkinsCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHallowedPumpkinsCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHallowedPumpkinsCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHallowedPumpkinsCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AHallowedPumpkinsCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AHallowedPumpkinsCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AHallowedPumpkinsCharacter::OnResetVR);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AHallowedPumpkinsCharacter::Sprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AHallowedPumpkinsCharacter::SprintStop);

	PlayerInputComponent->BindAction("Shoot", IE_Pressed, this, &AHallowedPumpkinsCharacter::Shoot);
	PlayerInputComponent->BindAction("Shoot", IE_Released, this, &AHallowedPumpkinsCharacter::ShootReleased);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AHallowedPumpkinsCharacter::Aim);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AHallowedPumpkinsCharacter::AimStop);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AHallowedPumpkinsCharacter::Interact);
	PlayerInputComponent->BindAction("ChangeWeapon", IE_Pressed, this, &AHallowedPumpkinsCharacter::ChangeWeapon);
}


void AHallowedPumpkinsCharacter::OnResetVR()
{
	// If HallowedPumpkins is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in HallowedPumpkins.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AHallowedPumpkinsCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AHallowedPumpkinsCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AHallowedPumpkinsCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AHallowedPumpkinsCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AHallowedPumpkinsCharacter::Jump()
{
	AimStop();

	Super::Jump();
}

void AHallowedPumpkinsCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);

		if (RefillingWater)
		{
			if (HasAuthority())
				RefillingWater = false;
			else StopRefill();
		}		
	}
}

void AHallowedPumpkinsCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);

		if (RefillingWater)
		{
			if (HasAuthority())
				RefillingWater = false;
			else StopRefill();
		}
	}
}

void AHallowedPumpkinsCharacter::Sprint()
{
	if (!Sprinting)
	{
		if (isReloading || (Stamina <= 0))
			return;

		RequestSprint();
	}
}

void AHallowedPumpkinsCharacter::SprintStop()
{
	if (Sprinting)
		RequestSprintStop();
}

void AHallowedPumpkinsCharacter::RequestSprint_Implementation()
{
	if (!Sprinting)
	{
		if (isReloading || (Stamina <= 0))
			return;

		SendSprint();
	}
}

void AHallowedPumpkinsCharacter::SendSprint_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	Sprinting = true;
}

void AHallowedPumpkinsCharacter::RequestSprintStop_Implementation()
{
	if (Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		Sprinting = false;
		SendSprintStop();
	}
}

void AHallowedPumpkinsCharacter::SendSprintStop_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	Sprinting = false;
}

void AHallowedPumpkinsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
		return;

	if (Dead)
		return;

	// Stamina regen
	if (!Sprinting)
	{
		if (Stamina < 100 && (StaminaRegenTimer == 0.f))
			StaminaRegenTimer = 0.5f;

		if (StaminaRegenTimer > 0.f)
		{
			if (StaminaRegenTimer > DeltaSeconds)
				StaminaRegenTimer -= DeltaSeconds;
			else
			{
				Stamina += 5;

				if (Stamina >= 100)
				{
					Stamina = 100;
					StaminaRegenTimer = 0.f;
				}
				else StaminaRegenTimer = 0.5f;
			}
		}
	}
	// Stamina regen end

	// Stamina loss
	if (Sprinting)
	{
		if (Stamina > 0 && (StaminaLossTimer == 0.f))
			StaminaLossTimer = 0.5f;

		if (StaminaLossTimer > 0.f)
		{
			if (StaminaLossTimer > DeltaSeconds)
				StaminaLossTimer -= DeltaSeconds;
			else
			{
				Stamina -= 5;

				if (Stamina <= 0)
				{
					Stamina = 0;
					StaminaLossTimer = 0.f;
					GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
					Sprinting = false;
					SendSprintStop();
				}
				else StaminaLossTimer = 0.5f;
			}
		}
	}
	// Stamina loss end

	// Water loss
	if (Watering)
	{
		if (Water > 0 && (WaterLossTimer == 0.f))
			WaterLossTimer = 0.5f;

		if (WaterLossTimer > 0.f)
		{
			if (WaterLossTimer > DeltaSeconds)
				WaterLossTimer -= DeltaSeconds;
			else
			{
				Water -= 1;

				WaterPumpkin();

				if (Water <= 0)
				{
					Water = 0;
					WaterLossTimer = 0.f;
					Watering = false;
				}
				else WaterLossTimer = 0.5f;
			}
		}
	}
	// Water loss end

	// Water regen
	if (RefillingWater && (WaterRegenTimer == 0.f))
		WaterRegenTimer = 0.5f;

	if (WaterRegenTimer > 0.f)
	{
		if (WaterRegenTimer > DeltaSeconds)
			WaterRegenTimer -= DeltaSeconds;
		else
		{
			Water += 5;

			if (Water >= 100)
			{
				Water = 100;
				WaterRegenTimer = 0.f;
				RefillingWater = false;
			}
			else
			{
				if (RefillingWater)
					WaterRegenTimer = 0.5f;
				else WaterRegenTimer = 0.f;
			}
		}
	}
	// Water regen end

	// Reload 
	if (ReloadTimer > 0.f)
	{
		if (ReloadTimer > DeltaSeconds)
			ReloadTimer -= DeltaSeconds;
		else
		{
			isReloading = false;
			ammo = 2;
			ReloadTimer = 0.f;
		}
	}
	// Reload end

	if (ShootCDTimer > 0.f)
	{
		if (ShootCDTimer > DeltaSeconds)
			ShootCDTimer -= DeltaSeconds;
		else
			ShootCDTimer = 0.f;
	}
}

void AHallowedPumpkinsCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	CrosshairWidget_i = CreateWidget<UUserWidget>(GetWorld(), CrosshairWidget);
	CrosshairWidget_i->AddToViewport();
}

void AHallowedPumpkinsCharacter::Restart()
{
	Super::Restart();

	if (IsLocallyControlled())
		if (AHPPlayerState* ps = Cast<AHPPlayerState>(GetPlayerState()))
			ps->RequestMonsterHiddenState();
}

void AHallowedPumpkinsCharacter::Shoot()
{
	if (GunEquipped)
	{
		if (!Aiming || (ammo < 1) || isReloading)
			return;

		RequestShoot();
	}
	else
	{
		if (Water > 0)
			RequestShoot();
	}
}

void AHallowedPumpkinsCharacter::ShootReleased()
{
	if (Watering)
		RequestShootStop();
}

void AHallowedPumpkinsCharacter::RequestShoot_Implementation()
{
	if (GunEquipped)
	{
		if (!Aiming || (ammo < 1) || isReloading || (ShootCDTimer != 0.f))
			return;

		FVector EyeLocation;
		FRotator EyeRotation;
		GetActorEyesViewPoint(EyeLocation, EyeRotation);
		FVector TraceEnd = EyeLocation + (EyeRotation.Vector() * 10000);

		FHitResult Fhit;
		FCollisionQueryParams params;
		params.AddIgnoredActor(this);
		bool impact = false;
		FTransform impactpos;
		if (GetWorld()->LineTraceSingleByChannel(Fhit, EyeLocation, TraceEnd, ECollisionChannel::ECC_Visibility, params))
		{
			impactpos.SetLocation(Fhit.Location);
			impactpos.SetRotation(Fhit.ImpactNormal.Rotation().Quaternion());
			impactpos.SetScale3D((FVector)(0.25f));

			impact = true;

			AActor* HitActor = Fhit.GetActor();
			if (AMonster* MonsterActor = Cast<AMonster>(HitActor))
				if (!MonsterActor->Dead)
					MonsterActor->Die();
		}

		SendShoot(impact, impactpos);
	}
	else
	{
		if (Water > 0)
		{
			FTransform nulltransform;
			SendShoot(false, nulltransform, true);
		}
	}
}

void AHallowedPumpkinsCharacter::SendShoot_Implementation(bool impact, FTransform impactpos, bool pwater)
{
	if (pwater)
	{
		Watering = true;
		RefillingWater = false;
		return;
	}

	--ammo;

	ShootCDTimer = 0.5f;
	JustFired = true;
	JustFiredBP();

	UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, WeaponMesh, FName(TEXT("Muzzle")), FVector(ForceInit), FRotator::ZeroRotator, ((FVector)(0.25f)));
	UGameplayStatics::SpawnSoundAttached(ShootSound, WeaponMesh, NAME_None);

	if(impact)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, impactpos);
}

void AHallowedPumpkinsCharacter::RequestShootStop_Implementation()
{
	if (Watering)
		SendShootStop();
}

void AHallowedPumpkinsCharacter::SendShootStop_Implementation()
{
	Watering = false;
}

void AHallowedPumpkinsCharacter::Aim()
{
	if (!GunEquipped || isReloading)
		return;

	RequestAim();
}

void AHallowedPumpkinsCharacter::AimStop()
{
	RequestAimStop();
}

void AHallowedPumpkinsCharacter::RequestAim_Implementation()
{
	if (!GunEquipped || isReloading)
		return;

	SendAim();
}

void AHallowedPumpkinsCharacter::SendAim_Implementation()
{
	Aiming = true;
	if(IsLocallyControlled())
		CrosshairWidget_i->SetVisibility(ESlateVisibility::Visible);
}

void AHallowedPumpkinsCharacter::RequestAimStop_Implementation()
{
	SendAimStop();
}

void AHallowedPumpkinsCharacter::SendAimStop_Implementation()
{
	Aiming = false;
	if (IsLocallyControlled())
		CrosshairWidget_i->SetVisibility(ESlateVisibility::Hidden);
}

void AHallowedPumpkinsCharacter::Interact()
{
	if (GunEquipped)
	{
		if (isReloading)
			return;

		TArray<AActor*> BoxResult;
		GetOverlappingActors(BoxResult, AAmmoBox::StaticClass());

		if (BoxResult.Num() > 0)
			RequestInteract();
	}
	else
	{
		TArray<AActor*> BucketResult;
		GetOverlappingActors(BucketResult, AWaterBucket::StaticClass());

		if (BucketResult.Num() > 0)
			RequestInteract();
	}
}

void AHallowedPumpkinsCharacter::RequestInteract_Implementation()
{
	if (GunEquipped)
	{
		if (isReloading)
			return;

		TArray<AActor*> BoxResult;
		GetOverlappingActors(BoxResult, AAmmoBox::StaticClass());

		if (BoxResult.Num() > 0)
		{
			AAmmoBox* ammb = Cast<AAmmoBox>(BoxResult[0]);
			if (ammb->Charges > 0)
			{
				SendSprintStop();
				SendAimStop();
				SendInteract(true);
				ReloadTimer = 4.f;

				--ammb->Charges;
			}
		}
	}
	else
	{
		TArray<AActor*> BucketResult;
		GetOverlappingActors(BucketResult, AWaterBucket::StaticClass());

		if (BucketResult.Num() > 0)
		{
			Watering = false;
			SendInteract(false);
		}
	}
}

void AHallowedPumpkinsCharacter::SendInteract_Implementation(bool gun)
{
	if (gun)
	{
		isReloading = true;
		UGameplayStatics::SpawnSoundAttached(ReloadSound, WeaponMesh, NAME_None);
	}
	else RefillingWater = true;
}

void AHallowedPumpkinsCharacter::ChangeWeapon()
{
	if (isReloading || Aiming || RefillingWater || Watering)
		return;

	RequestChangeWeapon();
}

void AHallowedPumpkinsCharacter::RequestChangeWeapon_Implementation()
{
	if (isReloading || Aiming || RefillingWater || Watering)
		return;

	if (GunEquipped)
		SendChangeWeapon(false);
	else
		SendChangeWeapon(true);
}

void AHallowedPumpkinsCharacter::SendChangeWeapon_Implementation(bool newvalue)
{
	GunEquipped = newvalue;
	EquipChanged(newvalue);
}

void AHallowedPumpkinsCharacter::Die_Implementation()
{
	Dead = true;
	Watering = false;

	if (HasAuthority() || IsLocallyControlled())
	{
		GetCharacterMovement()->DisableMovement();
		DisableInput(GetController<APlayerController>());
	}

	if (HasAuthority())
	{
		AHPGameState* gs = Cast<AHPGameState>(UGameplayStatics::GetGameState(GetWorld()));
		gs->HumanKilled();
	}
}

FVector AHallowedPumpkinsCharacter::GetPawnViewLocation() const
{
	if (FPScamera)
		return FPScamera->GetComponentLocation();

	return Super::GetPawnViewLocation();
}

bool AHallowedPumpkinsCharacter::IsActorFrontOfMe(AActor* OtherActor)
{
	FVector lookVec = GetActorRotation().Vector();
	FVector dirToTarget = OtherActor->GetActorLocation() - GetActorLocation();

	float dotp = FVector::DotProduct(lookVec, dirToTarget);

	if (dotp > 0)
		return true;
	else return false;
}

void AHallowedPumpkinsCharacter::WaterPumpkin()
{
	TArray<AActor*> Pumpkins;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APumpkin::StaticClass(), Pumpkins);

	if (!Pumpkins.Num())
		return;

	for (int32 i = 0; i < Pumpkins.Num();)
	{
		if (!IsActorFrontOfMe(Pumpkins[i]))
			Pumpkins.RemoveAt(i);
		else ++i;
	}

	if (!Pumpkins.Num())
		return;

	float DistanceResult = 0;
	AActor* nearestac = UGameplayStatics::FindNearestActor(GetActorLocation(), Pumpkins, DistanceResult);

	if (DistanceResult <= WaterRange)
	{
		APumpkin* Pumpkin = Cast<APumpkin>(nearestac);
		Pumpkin->IncreaseWetness(30.f);
	}
}

void AHallowedPumpkinsCharacter::StopRefill_Implementation()
{
	RefillingWater = false;
}

void AHallowedPumpkinsCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHallowedPumpkinsCharacter, Stamina);
	DOREPLIFETIME(AHallowedPumpkinsCharacter, isReloading);
	DOREPLIFETIME(AHallowedPumpkinsCharacter, ammo);
	DOREPLIFETIME(AHallowedPumpkinsCharacter, Water);
	DOREPLIFETIME(AHallowedPumpkinsCharacter, Watering);
	DOREPLIFETIME(AHallowedPumpkinsCharacter, RefillingWater);
}