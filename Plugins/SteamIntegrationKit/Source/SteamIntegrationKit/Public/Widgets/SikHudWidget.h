// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystem/SikSubsystem.h"
#include "SikHudWidget.generated.h"

class USikSessionDataWidget;

/**
 * Hud class implements the multiplayer sessions subsystem
 * Requests for all the multiplayer functionality and handles the callbacks
 ******************************************************************************************/
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Widgets))
class STEAMINTEGRATIONKIT_API USikHudWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	/** Function to initialize the widget */
	virtual bool Initialize() override;

#pragma region Core Functions
	
private:
	/**
	 * Called to request the SikSubsystem to host the game with the given settings
	 * 
	 * @param InSessionSettings settings to host the session with
	 */
	UFUNCTION(BlueprintCallable, Category = "SikHud")
	void HostGame(const FSikCustomSessionSettings& InSessionSettings);
	
	/**
	 * Called when user enters any session code he wishes to join
	 * Function requests the SikSubsystem to find all the active session
	 * Then it checks if any session is hosted with the entered code and then requests to join it 
	 * 
	 * @param InSessionCode: Session code entered by the user
	 */
	UFUNCTION(BlueprintCallable, Category = "SikHud")
	void EnterCode(const FText& InSessionCode);

#pragma endregion Core Functions
	
#pragma region Subsystem Callbacks

private:
	/**
	 * Callback from subsystem binding after completing session creation operation
	 *
	 * @param bWasSuccessful: True if session creation was successful
	 */
	UFUNCTION()
	void OnSessionCreatedCallback(bool bWasSuccessful);
	
	/**
	 * Callback from subsystem binding after completing finding sessions operation
	 *
	 * @param SessionResults: True if session creation was successful
	 * @param bWasSuccessful: True when the operation was successful
	 */
	void OnSessionsFoundCallback(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

	/**
	 * Callback from subsystem binding after completing session joining operation
	 *
	 * @param Result: Result of the session joining operation
	 */
	void OnSessionJoinedCallback(EOnJoinSessionCompleteResult::Type Result);

#pragma endregion Subsystem Callbacks

#pragma region Defaults
	
private:
	/**
	 * When sessions are found by SikSubsystem this function is called if player has requested to join the session via code
	 * This function then filters the result and checks is a session exists with given session code and joins it
	 * 
	 * @param SessionSearchResults 
	 */
	void JoinSessionViaSessionCode(const TArray<FOnlineSessionSearchResult>& SessionSearchResults);
	
	/** Updates the active session list if there is any change in active sessions */
	void UpdateSessionsList(const TArray<FOnlineSessionSearchResult>& Results);
	
	/** Called by UpdateSessionsList after update of list is completed to find new sessions */
	void FindNewSessionsIfAllowed();

	/**
	 * Filters and returns the entered session code so that the code does not exceed the max limit
	 * Also checks if all the characters entered are digits only
	 *
	 * @param InCode: The code entered by the user
	 * @return FText: The filtered out code that has to be displayed on the text box
	 */
	UFUNCTION(BlueprintCallable, Category = "Defaults")
	static FText FilterEnteredSessionCode(const FText& InCode);
	
	/** Subsystem that handles all the multiplayer functionality */
	UPROPERTY()
	TObjectPtr<USikSubsystem> SikSubsystem;

	/** Path to the lobby map, we will travel to this map after creating a session successfully */
	UPROPERTY(EditDefaultsOnly, Category = "Defaults")
	FString LobbyMapPath = FString("");

	/** Called when player opens the browse menu to start finding session only when browse menu is open */
	UFUNCTION(BlueprintCallable, Category = "Defaults")
	void StartFindingSessions();
	
	/** Called when player closes the browse menu to stop finding session only when browse menu is closed */
	UFUNCTION(BlueprintCallable, Category = "Defaults")
	void StopFindingSessions();
	
	/** Flag that allows to find sessions only when browse menu is open */
	bool bCanFindNewSessions = false;
	
	/** True when user has requested to join via session code */
	bool bJoinSessionViaCode = false;

	/** The session code that user wishes to join */
	FString SessionCodeToJoin = "";

	/** Widget class to add to the session data scroll box */
	UPROPERTY(EditDefaultsOnly, Category = "Defaults")
	TSubclassOf<USikSessionDataWidget> SessionDataWidgetClass;
	
	UPROPERTY()
	TMap<FString, USikSessionDataWidget*> ActiveSessionWidgets;

	/** 
	 * Stores the session keys which was discovered in the last iteration of find sessions
	 * Helps to compare if new keys are added then only add those and do not update the older
	 * sessions which has no change
	 */
	TSet<FString> LastSessionKeys;
	
	/** Getter for SikSubsystem */
	TObjectPtr<USikSubsystem> GetSikSubsystem();
	
public:
	/**
	 * Called from USikSessionDataWidget::OnJoinSessionButtonClicked when user clicks on the join button for the session he wishes to join
	 *
	 * @paran InSessionToJoin: The session user wishes to join
	 */
	void JoinTheGivenSession(FOnlineSessionSearchResult& InSessionToJoin);

#pragma endregion Defaults
	
#pragma region Blueprint Events
	
public:
	/** Displays a message on the Hud, if it is an error then also sets the message close to be visible
	 * 
	 * @param InMessage: Message to display
	 * @param bIsErrorMessage: True when it is an error message to that close button can be displayed
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	void ShowMessage(const FString& InMessage, bool bIsErrorMessage = false);

	/**
	 * Adds a session data widget to the scroll box of the session result widget
	 * Hides if any message is displayed and makes the widget switcher show the session result widget
	 * 
	 * @param InSessionDataWidget: The widget to add
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	void AddSessionDataWidget(USikSessionDataWidget* InSessionDataWidget);

	/** Sets visibility of the throbber depending on if any sessions are present or not */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	void SetFindSessionsThrobberVisibility(ESlateVisibility InSlateVisibility);

	/** 
	 * Gets the user set filter of the sessions so that only the sessions that user wishes to
	 * look for is only displayed in the result scroll box
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	FSikCustomSessionSettings GetCurrentSessionsFilter();

	/** Clears the result box in case of start or stop finding sessions */
	UFUNCTION(BlueprintImplementableEvent, Category = "Defaults")
	void ClearSessionsScrollBox();
	
#pragma endregion Blueprint Events
	
};
