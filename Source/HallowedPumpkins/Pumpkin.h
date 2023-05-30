// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pumpkin.generated.h"

UCLASS()
class HALLOWEDPUMPKINS_API APumpkin : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* Scene;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* Sphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* Mesh;
	
public:	
	// Sets default values for this actor's properties
	APumpkin();

	UPROPERTY(BluePrintReadOnly)
	int Wetness;

	UPROPERTY(BluePrintReadOnly, Replicated)
	bool Watered;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void IncreaseWetness(int amount);
};
