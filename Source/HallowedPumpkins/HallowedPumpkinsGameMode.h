// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HallowedPumpkinsGameMode.generated.h"

UCLASS(minimalapi)
class AHallowedPumpkinsGameMode : public AGameModeBase
{
	GENERATED_BODY()

	TArray<APlayerController*> PlayersWaiting;

public:
	AHallowedPumpkinsGameMode();

	void PlayerReady(APlayerController* player);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SpawnPlayer(APlayerController* player);

	UFUNCTION(BlueprintCallable)
	void SpawnWaitingPlayers();
};



