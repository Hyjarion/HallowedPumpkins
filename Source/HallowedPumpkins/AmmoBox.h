// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmmoBox.generated.h"

UCLASS()
class HALLOWEDPUMPKINS_API AAmmoBox : public AActor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* Sphere;
	
public:	
	// Sets default values for this actor's properties
	AAmmoBox();

	UPROPERTY(BlueprintReadOnly, Replicated)
	int Charges;

	UFUNCTION(BlueprintCallable)
	void InitCharges();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
