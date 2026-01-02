// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SikLobbyGameMode.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyPlayersChanged, uint32);

/**
 * Game mode for the lobby map, if any user joins or leave then updates that data
 * So that host can start session only when the required no of players are present
 ******************************************************************************************/
UCLASS(Blueprintable, BlueprintType, ClassGroup=GameMode)
class STEAMINTEGRATIONKIT_API ASikLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	/** Default Constructor to enable seamless travel */
	ASikLobbyGameMode(const FObjectInitializer& ObjectInitializer);
	
	/** Delegate UI (USikLobbyWidget) will bind to */
	static FOnLobbyPlayersChanged OnLobbyPlayersChangedGlobal;

protected:
	/** 
	 * Callback when a user joins or leaves the lobby, updates the CurrentLobbyPlayers and broadcasts the 
	 * OnLobbyPlayersChangedGlobal delegate so that num players can be updated and start session 
	 * button can be enabled if required players reached
	 */
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* ExitingController) override;
	
private:
	/** Stores the current no of players present in the lobby */
	uint32 CurrentLobbyPlayers = 0;
};
