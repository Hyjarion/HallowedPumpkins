// Copyright Epic Games, Inc. All Rights Reserved.

#include "HallowedPumpkinsGameMode.h"
#include "HallowedPumpkinsCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "HPGameState.h"
#include "Kismet/GameplayStatics.h"

AHallowedPumpkinsGameMode::AHallowedPumpkinsGameMode()
{
	// set default pawn class to our Blueprinted character
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/Human"));
	//if (PlayerPawnBPClass.Class != NULL)
	//{
	//	DefaultPawnClass = PlayerPawnBPClass.Class;
	//}

	GameStateClass = AHPGameState::StaticClass();
}

void AHallowedPumpkinsGameMode::PlayerReady(APlayerController* player)
{
	if (AHPGameState* hpgs = Cast<AHPGameState>(UGameplayStatics::GetGameState(GetWorld())))
	{
		if (hpgs->GameStarted)
			SpawnPlayer(player);		
		else
			PlayersWaiting.Add(player);
	}
}

void AHallowedPumpkinsGameMode::SpawnWaitingPlayers()
{
	TArray<AActor*> AllPlayers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), AllPlayers);

	TArray<APlayerController*> PlayersToSpawn;

	for (auto i : PlayersWaiting)
	{
		for (auto j : AllPlayers)
		{
			if (j == i)
			{
				PlayersToSpawn.Add(i);
				break;
			}
		}
	}

	for (auto i : PlayersToSpawn)
		SpawnPlayer(i);

	PlayersWaiting.Empty();
}
