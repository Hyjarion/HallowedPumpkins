// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HPPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class HALLOWEDPUMPKINS_API AHPPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AHPPlayerState();

	UPROPERTY(BluePrintReadWrite, Replicated)
	bool TeamMonster;

	UPROPERTY(BluePrintReadWrite, Replicated)
	bool isReady;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Ready(bool pTeamMonster);

	UFUNCTION(Server, Reliable)
	void RequestMonsterHiddenState();
};