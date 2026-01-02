// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#include "GameMode/SikLobbyGameMode.h"

#include "System/SikLogger.h"

ASikLobbyGameMode::ASikLobbyGameMode(const FObjectInitializer& ObjectInitializer)
{
	bUseSeamlessTravel = true;
}

FOnLobbyPlayersChanged ASikLobbyGameMode::OnLobbyPlayersChangedGlobal;

void ASikLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	LOG_INFO(TEXT("Player joined lobby"));

	CurrentLobbyPlayers += 1;
	
	OnLobbyPlayersChangedGlobal.Broadcast(CurrentLobbyPlayers);
}

void ASikLobbyGameMode::Logout(AController* ExitingController)
{
	Super::Logout(ExitingController);

	LOG_INFO(TEXT("Player left lobby"));

	CurrentLobbyPlayers -= 1;
	
	OnLobbyPlayersChangedGlobal.Broadcast(CurrentLobbyPlayers);
}
