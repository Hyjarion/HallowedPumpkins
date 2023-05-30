// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "HPGI.generated.h"

/**
 * 
 */
UCLASS()
class HALLOWEDPUMPKINS_API UHPGI : public UGameInstance
{
	GENERATED_BODY()

public:
	UHPGI();

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	int RequiredPumpkinsToWin;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	int PumpkinsCount;

	UPROPERTY(EditAnywhere, BluePrintReadWrite)
	int AICount;

	UPROPERTY(BluePrintReadOnly)
	int Port;

	UPROPERTY(BluePrintReadWrite)
	bool isMultiplayer;

	UFUNCTION(BlueprintCallable)
	void InviteSteamFriend(FString name);

	UFUNCTION(BlueprintCallable)
	void CreateSteamSession();

	virtual void Init() override;
	virtual void Shutdown() override;

private:
	void OnReadFriendsListCompleted(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorString);
	FOnReadFriendsListComplete FOnReadFriendsListCompleteDelegate;

	FDelegateHandle CreateCompleteDelegateHandle;
	FOnCreateSessionCompleteDelegate CreateCompleteDelegate;
	void OnCreateCompleted(FName SessionName, bool bWasSuccessful);

	FOnStartSessionCompleteDelegate StartCompleteDelegate;
	FDelegateHandle StartCompleteDelegateHandle;
	void OnStartCompleted(FName SessionName, bool bWasSuccessful);

	FOnSessionUserInviteAcceptedDelegate SessionInviteAcceptedDelegate;
	FDelegateHandle SessionInviteAcceptedDelegateHandle;
	void OnSessionInviteAccepted(const bool bWasSuccessful, int32 LocalPlayer, TSharedPtr<const FUniqueNetId> PersonInvited, const FOnlineSessionSearchResult& SessionToJoin);

	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	FString NameToFind;
};
