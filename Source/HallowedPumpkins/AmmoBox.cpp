// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoBox.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "HPGI.h"

// Sets default values
AAmmoBox::AAmmoBox()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));

	Charges = 1;
}

// Called when the game starts or when spawned
void AAmmoBox::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
		InitCharges();
}

void AAmmoBox::InitCharges()
{
	UHPGI* gi = Cast<UHPGI>(UGameplayStatics::GetGameInstance(GetWorld()));
	Charges = FGenericPlatformMath::RoundToInt((float)gi->AICount / 2.f);
}

// Called every frame
void AAmmoBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAmmoBox::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAmmoBox, Charges);
}

