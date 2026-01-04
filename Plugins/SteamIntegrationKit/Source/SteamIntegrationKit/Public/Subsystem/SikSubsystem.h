// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SikSubsystem.generated.h"

#define SETTING_NUMPLAYERSREQUIRED FName("NumPlayers") 
#define SETTING_FILTERSEED FName("FilterSeed")
#define SETTING_FILTERSEED_VALUE 94311
#define SETTING_SESSION_VISIBILITY FName("Visibility")
#define SETTING_SESSION_CODELENGTH 6

#pragma region Custom Delegates

/**
 * Custom delegates for the classes invoking this subsystem to bind to
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerSessionsOnCreateSessionComplete, bool, bWasSuccessful);
/** FOnlineSessionSearchResult is not UCLASS so we cannot use DYNAMIC keyword here */
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerSessionsOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
/** EOnJoinSessionCompleteResult is not UCLASS so we cannot use DYNAMIC keyword here */
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerSessionsOnJoinSessionsComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerSessionsOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerSessionsOnStartSessionComplete, bool, bWasSuccessful);

#pragma endregion Custom Delegates

/**
 * Structure to store all the settings to be set while creating a session
 ******************************************************************************************/
USTRUCT(Blueprintable, BlueprintType)
struct FSikCustomSessionSettings
{
	GENERATED_BODY()

	/** Name of the map that user has selected */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FString MapName = FString("");

	/** Game mode selected by the user */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FString GameMode = FString("");

	/** Numbers of players session will host */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FString Players = FString("");

	/** Whether the session is public or private */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FString Visibility = FString("");
};

/**
 * Class to handle all the session operations
 * Being a subsystem of game instance this can be called from anywhere
 ******************************************************************************************/
UCLASS(ClassGroup = (Subsystem))
class STEAMINTEGRATIONKIT_API USikSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/** Default Constructor */
	USikSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	virtual void Deinitialize() override;

#pragma region Custom Delegates Declaration

public:
	/**
	 * Custom delegates for classes to bind to
	 */
	FMultiplayerSessionsOnCreateSessionComplete MultiplayerSessionsOnCreateSessionComplete;
	FMultiplayerSessionsOnFindSessionsComplete MultiplayerSessionsOnFindSessionsComplete;
	FMultiplayerSessionsOnJoinSessionsComplete MultiplayerSessionsOnJoinSessionsComplete;
	FMultiplayerSessionsOnDestroySessionComplete MultiplayerSessionsOnDestroySessionComplete;
	FMultiplayerSessionsOnStartSessionComplete MultiplayerSessionsOnStartSessionComplete;
	
#pragma endregion Custom Delegates Declaration
	
#pragma region Session Operations

public:
	/**
	 * Creates a session for the host to join, Called from USikHUDWidget::HostGame
	 *
	 * @param InCustomSessionSettings: Custom session settings to create the session with
	 */
	void CreateSession(const FSikCustomSessionSettings& InCustomSessionSettings);

	/** 
	 * Called from USikHUDWidget to start finding sessions with the coded settings
	 * Finds sessions for the client to join to
	 */
	void FindSessions();

	/**
	 * Called from USikSubsystem::HandleAppExit and USikHUDWidget::JoinTheGivenSession
	 * Stops if any session finding operation is active
	 */
	void CancelFindSessions();
	
	/**
	 * Called from USikHUDWidget to join the session requested by the client
	 *
	 *  @param InSessionToJoin: Passed by the client after selecting the appropriate session he wishes to join
	 */
	void JoinSessions(FOnlineSessionSearchResult& InSessionToJoin);

	/** Called from USikLobbyWidget::OnStartGameClicked to start the actual session */
	void StartSession();
	
private:
	/** Destroys the currently active session */
	void DestroySession();

#pragma endregion Session Operations

#pragma region Session Operation Complete Delegates

private:
	/**
	 * Delegates for session interface to bind to so that we can get a callback on any session operation completion
	 */
	
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;
		
#pragma endregion Session Operation Complete Delegates
	
#pragma region Session Operations On Completion Delegates Callbacks
	
private:
	/** Called when a session is successfully created */
	void OnCreateSessionCompleteCallback(FName SessionName, bool bWasSuccessful);

	/** Called when sessions with given session settings are found */
	void OnFindSessionsCompleteCallback(bool bWasSuccessful);

	/** Called when a session is joined */
	void OnJoinSessionCompleteCallback(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/** Called when a session is destroyed */
	void OnDestroySessionCompleteCallback(FName SessionName, bool bWasSuccessful);

	/** Called when a session is destroyed */
	void OnStartSessionCompleteCallback(FName SessionName, bool bWasSuccessful);
	
#pragma endregion Session Operations On Completion Delegates Callbacks

#pragma region Defaults
	
private:
	/**
	 * This variable acts an access point to OnlineSubsystem's session interface
	 *
	 * Using this variable only I will be able to call for creation, destruction, joining and finding of sessions
	 * Initialized in the constructor
	 */
	IOnlineSessionPtr SessionInterface;

	/** Callback that calls HandleAppExit in case of any network connection failure */
	UFUNCTION()
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, 
		const FString& ErrorString);

	/**
	 * Called in case of network failure or if player exits the game or any similar scenario
	 * Destroys any active session and cancels if the find session is in progress
	 */
	void HandleAppExit();

	/**
	 * Generates and returns a random unique code to create a session with
	 * For the clients to later search and join a session with this unique code
	 *
	 * Appends the time in minutes, seconds and milliseconds and returns it as session code
	 *
	 * NOTE: Under extremely rare circumstances the generated code may not be unique and there might be a session with same codes
	 * 
	 * @return an FSting with a random unique code
	 */
	static FString GenerateSessionUniqueCode();

	/** True if subsystem is finding sessions */
	bool bFindSessionsInProgress = false;
	
	/** Stores the last created session search to get the search results */
	TSharedPtr<FOnlineSessionSearch> LastCreatedSessionSearch;
	
	/** True when user requests to create a new session but the last created session is already active */
	bool bCreateSessionOnDestroy = false;

	/**
	 * When a session is already active player selects to create a new session then this will store the settings
	 * As we will have to destroy the active session and then create a new one 
	 */
	FSikCustomSessionSettings SessionSettingsForTheSessionToCreateAfterDestruction;
	
	/** 
	 * @returns true if the session is in the given state
	 * @param State The session state to check
	 */
	bool IsSessionInState(EOnlineSessionState::Type State) const;
	
#pragma endregion Defaults
	
#pragma region Getter
	
public:
	/** 
	 * @returns true if successfully fetched max players 
	 * @param OutMaxPlayers the max number of players supported by the current session
	 */
	bool GetMaxPlayers(int32& OutMaxPlayers) const;

	/** 
	 * @returns true if successfully fetched the requested setting 
	 * @param InSettingName name of the setting to find
	 * @param OutSessionSetting the fetched setting value
	 */
	bool GetSessionSetting(const FName InSettingName, FString& OutSessionSetting) const;
	
#pragma endregion Getter
	
};
