// Fill out your copyright notice in the Description page of Project Settings.


#include "Pumpkin.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HPGameState.h"
#include "Net/UnrealNetwork.h"

// Sets default values
APumpkin::APumpkin()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	RootComponent = Scene;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(RootComponent);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Sphere);

	Wetness = 0;
	Watered = false;
}

// Called when the game starts or when spawned
void APumpkin::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APumpkin::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APumpkin::IncreaseWetness(int amount)
{
	if (Watered)
		return;

	Wetness += amount;

	if (Wetness >= 100)
	{
		Watered = true;
		AHPGameState* hps = Cast<AHPGameState>(UGameplayStatics::GetGameState(GetWorld()));
		hps->PumpkinWatered();
	}
}

void APumpkin::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APumpkin, Watered);
}