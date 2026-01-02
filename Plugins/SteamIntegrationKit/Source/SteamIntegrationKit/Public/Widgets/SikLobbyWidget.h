// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystem/SikSubsystem.h"
#include "SikLobbyWidget.generated.h"

class UHorizontalBox;
class UTextBlock;
class UButton;

/**
 * Lobby widget shown after joining the lobby map.
 * 
 * - Host (listen server):
 *      * Sees session code
 *      * Sees Start button (disabled until lobby full)
 *      * Clicking Start → calls USikSubsystem::StartSession()
 * 
 * - Client:
 *      * Sees session code only (no Start button / disabled)
 ******************************************************************************************/
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Widgets))
class STEAMINTEGRATIONKIT_API USikLobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	/** Sets up the widget */
	virtual bool Initialize() override;
    
	/** Removes any active bindings */
	virtual void NativeDestruct() override;

#pragma region Components
    
private:
	/** Displays the session code of the current session */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SessionCodeText;

	/** 
	 * Box containing the number of players in the session and the max players session can handle
	 * Only visible to the host
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> PlayersCountDataHb;

	/** Text to display the actual player count number; Only visible to the host */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerCountText;
    
	/** Display whether the session is private or public */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SessionVisibilityText;
    
	/** Displays the current game mode of the session */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GameModeText;
    
	/** Displays the selected map for the session */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MapNameText;
    
	/** Start Game button (host only) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> StartGameButton;

#pragma endregion Components
    
#pragma region Bindings
    
private:
	/** Refresh lobby UI every time a player joins or leaves the session, delegate callback function */
	UFUNCTION()
	void OnLobbyPlayersChangedGlobal(const uint32 CurrentPlayers);

	/** Callback when host clicks the starts button */
	UFUNCTION(BlueprintCallable)
	void OnStartGameClicked();

	/** Callback when start session is completed */
	UFUNCTION()
	void OnSessionStartedCallback(bool bWasSuccessful);

#pragma endregion Bindings
    
#pragma region Defaults
    
private:
	/** Path to gameplay map (e.g. "/Game/Maps/Map_Gameplay") */
	UPROPERTY(EditDefaultsOnly, Category = "Multiplayer Sessions Subsystem")
	TMap<FString, FString> MapPaths;

	/** Cached subsystem pointer */
	UPROPERTY()
	TObjectPtr<USikSubsystem> SikSubsystem;

	/** True if this widget belongs to the listen server host */
	bool bIsHost = false;

	/** Flag to only update the session setting only once, as player count is updated whenever any player leaves or joins */
	bool bIsOtherSessionSettingsSet = false;
    
	/** Getter for SikSubsystem */
	TObjectPtr<USikSubsystem> GetSikSubsystem();
    
#pragma endregion Defaults
    
};
