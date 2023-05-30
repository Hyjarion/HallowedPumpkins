// Fill out your copyright notice in the Description page of Project Settings.


#include "HPGI.h"
#include "Online.h"
#include "OnlineSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

UHPGI::UHPGI()
{
	RequiredPumpkinsToWin = 50;
	PumpkinsCount = 500;
	AICount = 10;
	GConfig->GetInt(TEXT("URL"), TEXT("Port"), Port, GEngineIni);
	isMultiplayer = true;

	FOnReadFriendsListCompleteDelegate = FOnReadFriendsListComplete::CreateUObject(this, &ThisClass::OnReadFriendsListCompleted);
	CreateCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateCompleted);
	StartCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartCompleted);
	SessionInviteAcceptedDelegate = FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionInviteAccepted);
	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete);
}

void UHPGI::InviteSteamFriend(FString name)
{
	IOnlineFriendsPtr Friends = Online::GetFriendsInterface();

	if (Friends.IsValid())
	{
		ULocalPlayer* Player = Cast<ULocalPlayer>(UGameplayStatics::GetPlayerController(GetWorld(), 0)->Player);

		Friends->ReadFriendsList(Player->GetControllerId(), EFriendsLists::ToString((EFriendsLists::Default)), FOnReadFriendsListCompleteDelegate);
		NameToFind = name;
	}
}

void UHPGI::OnReadFriendsListCompleted(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorString)
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();

	if (bWasSuccessful)
	{
		IOnlineFriendsPtr Friends = Online::GetFriendsInterface();
		if (Friends.IsValid())
		{
			TArray< TSharedRef<FOnlineFriend> > FriendList;
			Friends->GetFriendsList(LocalUserNum, ListName, FriendList);

			for (int32 i = 0; i < FriendList.Num(); i++)
			{
				TSharedRef<FOnlineFriend> Friend = FriendList[i];

				if(Friend->GetRealName() == NameToFind)
				{
					ULocalPlayer* Player = Cast<ULocalPlayer>(UGameplayStatics::GetPlayerController(GetWorld(), 0)->Player);
					SessionInterface->SendSessionInviteToFriend(Player->GetControllerId(), NAME_GameSession, *Friend->GetUserId());
					return;
				}
			}
		}
	}
}

void UHPGI::CreateSteamSession()
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();

	FUniqueNetIdPtr UserID = UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayerState->GetUniqueId().GetUniqueNetId();

	if (SessionInterface.IsValid())
	{
		CreateCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegate);

		FOnlineSessionSettings Settings;
		Settings.NumPublicConnections = 100;
		Settings.NumPrivateConnections = 100;
		Settings.bShouldAdvertise = true;
		Settings.bAllowJoinInProgress = true;
		Settings.bIsLANMatch = false;
		Settings.bAllowJoinViaPresence = true;
		Settings.bIsDedicated = false;
		Settings.bUsesPresence = true;
		Settings.bUseLobbiesIfAvailable = true;
		Settings.bUseLobbiesVoiceChatIfAvailable = true;
		Settings.bAllowJoinViaPresenceFriendsOnly = false;
		Settings.bAntiCheatProtected = false;
		Settings.bUsesStats = false;
		Settings.bAllowInvites = true;

		SessionInterface->CreateSession(*UserID, NAME_GameSession, Settings);
		return;
	}
}

void UHPGI::OnCreateCompleted(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegateHandle);

		if (bWasSuccessful)
		{
			StartCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartCompleteDelegate);
			SessionInterface->StartSession(NAME_GameSession);
			return;
		}
	}
}

void UHPGI::OnStartCompleted(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();

	if (SessionInterface.IsValid())
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartCompleteDelegateHandle);
	

	if (bWasSuccessful)
	{
		
	}
}

void UHPGI::OnSessionInviteAccepted(const bool bWasSuccessful, int32 LocalPlayer, TSharedPtr<const FUniqueNetId> PersonInvited, const FOnlineSessionSearchResult& SessionToJoin)
{
	if(bWasSuccessful)
	{ 
		IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();

		if (SessionInterface.IsValid())
		{
			FUniqueNetIdPtr UserID = UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayerState->GetUniqueId().GetUniqueNetId();

			JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
			SessionInterface->JoinSession(*UserID, NAME_GameSession, SessionToJoin);

		}
	}
}

void UHPGI::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		if (Result == EOnJoinSessionCompleteResult::Success)
		{
			// Client travel to the server
			FString ConnectString;
			if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
			{
				UE_LOG_ONLINE_SESSION(Log, TEXT("Join session: traveling to %s"), *ConnectString);
				UGameplayStatics::GetPlayerController(GetWorld(), 0)->ClientTravel(ConnectString, TRAVEL_Absolute);
			}
		}
	}
}

void UHPGI::Init()
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();

	if (SessionInterface.IsValid())
	{
		SessionInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(SessionInviteAcceptedDelegate);
	}
}

void UHPGI::Shutdown()
{
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface();

	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionInviteAcceptedDelegateHandle);
	}

	Super::Shutdown();
}