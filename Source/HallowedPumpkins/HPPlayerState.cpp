// Fill out your copyright notice in the Description page of Project Settings.


#include "HPPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "HallowedPumpkinsGameMode.h"
#include "Monster.h"

AHPPlayerState::AHPPlayerState()
{
	TeamMonster = false;
	isReady = false;
}

void AHPPlayerState::Ready_Implementation(bool pTeamMonster)
{
	if (isReady)
		return;
	isReady = true;

	TeamMonster = pTeamMonster;

	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), Players);

	APlayerController* thisPlayer = nullptr;

	for (auto i : Players)
	{
		APlayerController* ic = Cast<APlayerController>(i);

		if (ic->PlayerState == this)
		{
			thisPlayer = ic;
			break;
		}
	}

	if (!thisPlayer)
		return;

	if (AHallowedPumpkinsGameMode* hpgm = Cast<AHallowedPumpkinsGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		hpgm->PlayerReady(thisPlayer);
	}
}

void AHPPlayerState::RequestMonsterHiddenState_Implementation()
{
	TArray<AActor*> monsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonster::StaticClass(), monsters);

	for (auto i : monsters)
	{
		AMonster* monster = Cast<AMonster>(i);
		monster->SendInteract(monster->hidden);
	}
}

void AHPPlayerState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHPPlayerState, TeamMonster);
	DOREPLIFETIME(AHPPlayerState, isReady);
}
