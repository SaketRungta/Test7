// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#include "Subsystem/SikSubsystem.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "System/SikLogger.h"

DEFINE_LOG_CATEGORY(SteamIntegrationKitLog);

USikSubsystem::USikSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionCompleteCallback)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsCompleteCallback)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionCompleteCallback)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionCompleteCallback)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionCompleteCallback))
{
	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		LOG_ERROR(TEXT("USikSubsystem::USikSubsystem No Online Subsystem detected! Ensure a valid subsystem is enabled."));
		return;
	}

	SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("USikSubsystem::USikSubsystem Online Subsystem does not support sessions!"));
	}

	FCoreDelegates::OnPreExit.AddUObject(this, &USikSubsystem::HandleAppExit);
}

void USikSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &USikSubsystem::HandleNetworkFailure);
	}
}

void USikSubsystem::Deinitialize()
{
	Super::Deinitialize();
	
	LOG_WARNING(TEXT("USikSubsystem::Deinitialize called"));

	HandleAppExit();
}

#pragma region Session Operations

void USikSubsystem::CreateSession(const FSikCustomSessionSettings& InCustomSessionSettings)
{
	LOG_INFO(TEXT("Called"));

	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("CreateSession SessionInterface is INVALID"));
		MultiplayerSessionsOnCreateSessionComplete.Broadcast(false);
		return;
	}
	
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{		
		LOG_ERROR(TEXT("NAME_GameSession already exists, destroying before creating a new one"));
		
		bCreateSessionOnDestroy = true;
		SessionSettingsForTheSessionToCreateAfterDestruction = InCustomSessionSettings;
		
		DestroySession();
		
		return;
	}

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	int32 NumPublicConnections = 2;
	if (InCustomSessionSettings.Players == "2v2") NumPublicConnections = 4;
	else if (InCustomSessionSettings.Players == "4v4") NumPublicConnections = 8;
	
	const TSharedPtr<FOnlineSessionSettings> OnlineSessionSettings = MakeShareable(new FOnlineSessionSettings());
	OnlineSessionSettings->bIsLANMatch = false;
	OnlineSessionSettings->NumPublicConnections = NumPublicConnections;
	OnlineSessionSettings->bAllowJoinInProgress = true;
	OnlineSessionSettings->bAllowJoinViaPresence = true;
	OnlineSessionSettings->bShouldAdvertise = true;
	OnlineSessionSettings->bUsesPresence = true;
	OnlineSessionSettings->bUseLobbiesIfAvailable = true;
	OnlineSessionSettings->Set(SETTING_FILTERSEED, SETTING_FILTERSEED_VALUE, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	OnlineSessionSettings->Set(SETTING_MAPNAME, InCustomSessionSettings.MapName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	OnlineSessionSettings->Set(SETTING_GAMEMODE, InCustomSessionSettings.GameMode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	OnlineSessionSettings->Set(SETTING_NUMPLAYERSREQUIRED, InCustomSessionSettings.Players, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	OnlineSessionSettings->Set(SETTING_SESSION_VISIBILITY, InCustomSessionSettings.Visibility, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	OnlineSessionSettings->Set(SETTING_SESSIONKEY, GenerateSessionUniqueCode(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (!SessionInterface->CreateSession(*GetWorld()->GetFirstLocalPlayerFromController()->GetPreferredUniqueNetId(), NAME_GameSession, *OnlineSessionSettings))
	{
		LOG_ERROR(TEXT("CreateSession failed to execute create session"));

		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		MultiplayerSessionsOnCreateSessionComplete.Broadcast(false);
	}
}

void USikSubsystem::FindSessions()
{
	LOG_INFO(TEXT("Called"));
	
	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("FindSessions SessionInterface is INVALID"));
		MultiplayerSessionsOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	
	if (bFindSessionsInProgress)
	{
		LOG_INFO(TEXT("Find session already in progress calling to cancel search"));
		CancelFindSessions();
	}
	
	bFindSessionsInProgress = true;
	
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	
	LastCreatedSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastCreatedSessionSearch->MaxSearchResults = 10000;
	LastCreatedSessionSearch->bIsLanQuery = false;
	LastCreatedSessionSearch->QuerySettings.Set(SETTING_FILTERSEED, SETTING_FILTERSEED_VALUE, EOnlineComparisonOp::Equals);
	LastCreatedSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	
	if (!GetWorld() || GetWorld()->bIsTearingDown)
	{
		LOG_WARNING(TEXT("FindSessions aborted – world is tearing down"));
		MultiplayerSessionsOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	if (!SessionInterface->FindSessions(*GetWorld()->GetFirstLocalPlayerFromController()->GetPreferredUniqueNetId(), LastCreatedSessionSearch.ToSharedRef()))
	{
		LOG_ERROR(TEXT("Call to session interface find sessions function failed"));
		
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		bFindSessionsInProgress = false;
		MultiplayerSessionsOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void USikSubsystem::CancelFindSessions()
{
	LOG_INFO(TEXT("Called"));

	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("SessionInterface is INVALID"));
		return;
	}

	bFindSessionsInProgress = false;
	
	LOG_WARNING(TEXT("Aborting search"));

	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	MultiplayerSessionsOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), true);
}

void USikSubsystem::JoinSessions(FOnlineSessionSearchResult& InSessionToJoin)
{
	LOG_INFO(TEXT("Called"));
	
	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("SessionInterface is INVALID"));
		MultiplayerSessionsOnJoinSessionsComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}
	
	if (IsSessionInState(EOnlineSessionState::Creating) ||
		IsSessionInState(EOnlineSessionState::Starting) ||
		IsSessionInState(EOnlineSessionState::Ending))
	{
		LOG_ERROR(TEXT("JoinSession blocked: session busy"));
		MultiplayerSessionsOnJoinSessionsComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	InSessionToJoin.Session.SessionSettings.bUseLobbiesIfAvailable = true;
	InSessionToJoin.Session.SessionSettings.bUsesPresence = true;
	
	if (!SessionInterface->JoinSession(*GetWorld()->GetFirstLocalPlayerFromController()->GetPreferredUniqueNetId(), NAME_GameSession, InSessionToJoin))
	{
		LOG_ERROR(TEXT("Call to session interface join session function failed"));
		
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerSessionsOnJoinSessionsComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void USikSubsystem::DestroySession()
{
	LOG_INFO(TEXT("Called"));
	
	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("SessionInterface is INVALID"));
		MultiplayerSessionsOnDestroySessionComplete.Broadcast(false);
		return;
	}

	if (!IsSessionInState(EOnlineSessionState::Pending) &&
		!IsSessionInState(EOnlineSessionState::InProgress) &&
		!IsSessionInState(EOnlineSessionState::Ended))
	{
		LOG_ERROR(TEXT("DestroySession failed: no session to destroy"));
		MultiplayerSessionsOnDestroySessionComplete.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		LOG_ERROR(TEXT("Call to session interface destroy session function failed"));

		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerSessionsOnDestroySessionComplete.Broadcast(false);
	}
}

void USikSubsystem::StartSession()
{
	LOG_INFO(TEXT("USikSubsystem::StartSession Called"));

	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("StartSession SessionInterface is INVALID"));
		MultiplayerSessionsOnStartSessionComplete.Broadcast(false);
		return;
	}
	
	if (!IsSessionInState(EOnlineSessionState::Pending))
	{
		LOG_ERROR(TEXT("StartSession called but session is NOT in Pending state"));
		MultiplayerSessionsOnStartSessionComplete.Broadcast(false);
		return;
	}

	StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

	if (!SessionInterface->StartSession(NAME_GameSession))
	{
		LOG_ERROR(TEXT("Call to session interface start session function failed"));

		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
		MultiplayerSessionsOnStartSessionComplete.Broadcast(false);
	}
}

#pragma endregion Session Operations

#pragma region Session Operations On Completion Delegates Callbacks
	
void USikSubsystem::OnCreateSessionCompleteCallback(FName SessionName, bool bWasSuccessful)
{
	LOG_INFO(TEXT("Created session : %s"), bWasSuccessful ? TEXT("success") : TEXT("failed"));

	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle); 
	}

	MultiplayerSessionsOnCreateSessionComplete.Broadcast(bWasSuccessful);	
}

void USikSubsystem::OnFindSessionsCompleteCallback(bool bWasSuccessful)
{
	LOG_INFO(TEXT("Found sessions : %s"), bWasSuccessful ? TEXT("success") : TEXT("failed"));

	bFindSessionsInProgress = false;
	
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	if (!LastCreatedSessionSearch.IsValid())
	{
		LOG_ERROR(TEXT("LastCreatedSessionSearch is Invalid"));
		MultiplayerSessionsOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), bWasSuccessful);
		return;
	}
		
	if (LastCreatedSessionSearch->SearchResults.IsEmpty())
	{
		LOG_WARNING(TEXT("Search result is empty no session found"));
		MultiplayerSessionsOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), bWasSuccessful);
		return;
	}

	MultiplayerSessionsOnFindSessionsComplete.Broadcast(LastCreatedSessionSearch->SearchResults, bWasSuccessful);
}

void USikSubsystem::OnJoinSessionCompleteCallback(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	switch (Result)
	{
	case EOnJoinSessionCompleteResult::Success:
		LOG_INFO(TEXT("Success"));
		break;
	case EOnJoinSessionCompleteResult::SessionIsFull:
		LOG_INFO(TEXT("SessionIsFull"));
		break;
	case EOnJoinSessionCompleteResult::SessionDoesNotExist:
		LOG_INFO(TEXT("SessionDoesNotExist"));
		break;
	case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
		LOG_INFO(TEXT("CouldNotRetrieveAddress"));
		break;
	case EOnJoinSessionCompleteResult::AlreadyInSession:
		LOG_INFO(TEXT("AlreadyInSession"));
		break;
	case EOnJoinSessionCompleteResult::UnknownError:
		LOG_INFO(TEXT("UnknownError"));
		break;
	}
	
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		LOG_WARNING(TEXT("Join failed, forcing local session cleanup"));

		if (SessionInterface && SessionInterface->GetNamedSession(NAME_GameSession))
		{
			SessionInterface->DestroySession(NAME_GameSession);
		}

		MultiplayerSessionsOnJoinSessionsComplete.Broadcast(Result);
		return;
	}
	
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	MultiplayerSessionsOnJoinSessionsComplete.Broadcast(Result);
}

void USikSubsystem::OnDestroySessionCompleteCallback(FName SessionName, bool bWasSuccessful)
{
	LOG_INFO(TEXT("Destroy session : %s"), bWasSuccessful ? TEXT("success") : TEXT("failed"));

	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(SessionSettingsForTheSessionToCreateAfterDestruction);
	}
	
	MultiplayerSessionsOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void USikSubsystem::OnStartSessionCompleteCallback(FName SessionName, bool bWasSuccessful)
{
	LOG_INFO(TEXT("Start session : %s | Success: %s"),
		*SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}

	MultiplayerSessionsOnStartSessionComplete.Broadcast(bWasSuccessful);
}

#pragma endregion Session Operations On Completion Delegates Callbacks

#pragma region Defaults
	
void USikSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	LOG_INFO(TEXT("Called"));
	
	HandleAppExit();
}

void USikSubsystem::HandleAppExit()
{	
	LOG_INFO(TEXT("Called"));

	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		LOG_WARNING(TEXT("Active session detected during shutdown. Destroying..."));
		DestroySession();
	}
	
	if (bFindSessionsInProgress)
	{
		CancelFindSessions();
	}
}

FString USikSubsystem::GenerateSessionUniqueCode()
{
	LOG_INFO(TEXT("Called"));
	
	const FGuid NewGuid = FGuid::NewGuid();
	const uint64 Part1 = (static_cast<uint64>(NewGuid.A) << 32) | static_cast<uint64>(NewGuid.B);
	const uint64 Part2 = (static_cast<uint64>(NewGuid.C) << 32) | static_cast<uint64>(NewGuid.D);
	uint64 GuidValue = Part1 ^ Part2;

	const FString AllowedChars = TEXT("BCDFGHJKLMNPQRSTVWXZ");
	const uint64 Base = AllowedChars.Len();

	FString Code;
	Code.Reserve(SETTING_SESSION_CODELENGTH);

	for (int32 i = 0; i < SETTING_SESSION_CODELENGTH; i++)
	{
		const int32 CharIndex = GuidValue % Base;
		Code.AppendChar(AllowedChars[CharIndex]);
		GuidValue /= Base;
	}

	LOG_INFO(TEXT("%s"), *Code);
	
	return Code;
}

bool USikSubsystem::IsSessionInState(EOnlineSessionState::Type State) const
{
	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("SessionInterface is INVALID"));
		return false;
	}

	const FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	if (!Session)
	{
		LOG_WARNING(TEXT("No active session found"));
		return false;
	}

	return Session->SessionState == State;
}

#pragma endregion Defaults
	
#pragma region Getter
	
bool USikSubsystem::GetMaxPlayers(int32& OutMaxPlayers) const
{
	OutMaxPlayers = 0;

	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("SessionInterface is INVALID"));
		return false;
	}

	const FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	if (!Session)
	{
		LOG_WARNING(TEXT("No active session found"));
		return false;
	}

	const int32 NumPublicConnections = Session->SessionSettings.NumPublicConnections;

	OutMaxPlayers = NumPublicConnections;

	return true;
}

bool USikSubsystem::GetSessionSetting(const FName InSettingName, FString& OutSessionSetting) const
{
	OutSessionSetting.Reset();

	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("SessionInterface is INVALID"));
		return false;
	}

	const FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	if (!Session)
	{
		LOG_WARNING(TEXT("No active session found"));
		return false;
	}

	FString SessionSetting;
	if (!Session->SessionSettings.Get(InSettingName, SessionSetting))
	{
		LOG_WARNING(TEXT("Failed to get %s from session settings"), *InSettingName.ToString());
		return false;
	}

	OutSessionSetting = SessionSetting;
	return true;
}

#pragma endregion Getter
