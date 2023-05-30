// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HPGameState.generated.h"

/**
 * 
 */
UCLASS()
class HALLOWEDPUMPKINS_API AHPGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AHPGameState();

	UFUNCTION()
	void PumpkinWatered();

	UFUNCTION(BlueprintImplementableEvent)
	void EndGame(bool monsterwinner, bool waterwin);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateWidgetPumpkinState();

	UFUNCTION()
	void MonsterKilled();
	UFUNCTION()
	void HumanKilled();

	UPROPERTY(BluePrintReadWrite, Replicated)
	bool GameStarted;

	UPROPERTY(BluePrintReadWrite, Replicated)
	int WateredPumpkins;
};
