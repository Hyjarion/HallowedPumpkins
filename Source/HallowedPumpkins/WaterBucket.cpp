// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterBucket.h"
#include "Components/SphereComponent.h"

// Sets default values
AWaterBucket::AWaterBucket()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
}

// Called when the game starts or when spawned
void AWaterBucket::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWaterBucket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

