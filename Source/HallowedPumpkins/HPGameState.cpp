// Fill out your copyright notice in the Description page of Project Settings.


#include "HPGameState.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "HPGI.h"
#include "HallowedPumpkinsCharacter.h"
#include "Monster.h"

AHPGameState::AHPGameState()
{
	WateredPumpkins = 0;
	GameStarted = false;
}

void AHPGameState::PumpkinWatered()
{
	++WateredPumpkins;

	UHPGI* gi = Cast<UHPGI>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (WateredPumpkins == gi->RequiredPumpkinsToWin)
		EndGame(false, true);
	else UpdateWidgetPumpkinState();
}

void AHPGameState::MonsterKilled()
{
	TArray<AActor*> Monsters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonster::StaticClass(), Monsters);

	bool SomeoneAlive = false;
	for (auto i : Monsters)
	{
		AMonster* monster = Cast<AMonster>(i);
		if (!monster->Dead)
		{
			SomeoneAlive = true;
			break;
		}
	}

	if (!SomeoneAlive)
		EndGame(false, false);
}

void AHPGameState::HumanKilled()
{
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHallowedPumpkinsCharacter::StaticClass(), Players);

	bool SomeoneAlive = false;
	for (auto i : Players)
	{
		AHallowedPumpkinsCharacter* human = Cast<AHallowedPumpkinsCharacter>(i);
		if (!human->Dead)
		{
			SomeoneAlive = true;
			break;
		}
	}

	if (!SomeoneAlive)
		EndGame(true, false);
}

void AHPGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHPGameState, WateredPumpkins);
	DOREPLIFETIME(AHPGameState, GameStarted);
}