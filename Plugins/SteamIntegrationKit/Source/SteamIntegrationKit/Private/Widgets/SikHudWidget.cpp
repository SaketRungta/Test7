// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#include "Widgets/SikHudWidget.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Widgets/SikSessionDataWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Online/OnlineSessionNames.h"
#include "System/SikLogger.h"

bool USikHudWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
	
	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
	
	if (!IsDesignTime())
	{
		if (!IsValid(GetSikSubsystem()))
		{
			LOG_ERROR(TEXT("Invalid GameInstance"));
			return true;
		}
	
		SikSubsystem->MultiplayerSessionsOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnSessionCreatedCallback);
		SikSubsystem->MultiplayerSessionsOnFindSessionsComplete.AddUObject(this, &ThisClass::OnSessionsFoundCallback);
		SikSubsystem->MultiplayerSessionsOnJoinSessionsComplete.AddUObject(this, &ThisClass::OnSessionJoinedCallback);
	}
	
	return true;
}

#pragma region Core Functions
	
void USikHudWidget::HostGame(const FSikCustomSessionSettings& InSessionSettings)
{
	LOG_INFO(TEXT("Called"));
	
	ShowMessage(FString("Hosting Game"));

	if (GetSikSubsystem())
	{
		SikSubsystem->CreateSession(InSessionSettings);
	}
}

void USikHudWidget::EnterCode(const FText& InSessionCode)
{
	LOG_INFO(TEXT("Called session Code Entered : %s"), *InSessionCode.ToString());
	
	bJoinSessionViaCode = true;
	SessionCodeToJoin = InSessionCode.ToString();

	if (SessionCodeToJoin.Len() < SETTING_SESSION_CODELENGTH)
	{
		ShowMessage(FString::Printf(TEXT("Room code must be %d digits long"), 
			SETTING_SESSION_CODELENGTH), true);		
		return;
	}
	
	ShowMessage(FString("Finding room"));
	
	if (GetSikSubsystem())
	{
		SikSubsystem->FindSessions();
	}
}

#pragma endregion Core Functions
	
#pragma region Subsystem Callbacks

void USikHudWidget::OnSessionCreatedCallback(bool bWasSuccessful)
{
	LOG_INFO(TEXT("Session created : %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failed"));
	
	if (!bWasSuccessful)
	{
		ShowMessage(FString("Failed to Create Session"), true);
		return;
	}

	const FString TravelPath = LobbyMapPath + FString("?listen");
	LOG_INFO(TEXT("Server travel to path: %s"), *TravelPath);
		
	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(TravelPath);
	}
}

void USikHudWidget::OnSessionsFoundCallback(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	LOG_INFO(TEXT("Session found : %s"), bWasSuccessful ? TEXT("Success") : TEXT("Failed"));
	
	if (!GetSikSubsystem())
	{		
		LOG_ERROR(TEXT("MultiplayerSessionsSubsystem is INVALID"));
		
		bJoinSessionViaCode = false;
		ShowMessage(FString("Unknown Error"), true);
		SetFindSessionsThrobberVisibility(ESlateVisibility::Visible);
		FindNewSessionsIfAllowed();
		return;
	}
	
	if (!bWasSuccessful)
	{
		LOG_ERROR(TEXT("Session search result unsuccessful"));
		
		bJoinSessionViaCode = false;
		SetFindSessionsThrobberVisibility(ESlateVisibility::Visible);
		FindNewSessionsIfAllowed();
		return;
	}

	if (bJoinSessionViaCode)
	{
		JoinSessionViaSessionCode(SessionResults);
	}
	else
	{
		UpdateSessionsList(SessionResults);
	}
}

void USikHudWidget::OnSessionJoinedCallback(EOnJoinSessionCompleteResult::Type Result)
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

	if (Result != EOnJoinSessionCompleteResult::Type::Success)
	{
		ShowMessage(FString::Printf(TEXT("%s"), LexToString(Result)), true);
		bJoinSessionViaCode = false;
		
		return;
	}
	
	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		LOG_ERROR(TEXT("OnlineSubsystem is NULL"));
	}
	
	const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		LOG_ERROR(TEXT("SessionInterface is INVALID"));
	}
	
	if (FString AddressOfSessionToJoin; 
			SessionInterface->GetResolvedConnectString(NAME_GameSession, AddressOfSessionToJoin))
	{
		if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
		{
			PlayerController->ClientTravel(AddressOfSessionToJoin, ETravelType::TRAVEL_Absolute);	
		}
	}
	else
	{
		LOG_ERROR(TEXT("Failed to find the address of the session to join"));
		
		ShowMessage(FString("Failed to Join Session"), true);
		bJoinSessionViaCode = false;
	}
}

#pragma endregion Subsystem Callbacks

#pragma region Defaults
	
void USikHudWidget::JoinSessionViaSessionCode(const TArray<FOnlineSessionSearchResult>& SessionSearchResults)
{
	LOG_INFO(TEXT("Called"));
	
	for (FOnlineSessionSearchResult CurrentSessionSearchResult : SessionSearchResults)
	{
		FString CurrentSessionResultSessionCode = FString(""); 
		CurrentSessionSearchResult.Session.SessionSettings.Get(SETTING_SESSIONKEY, CurrentSessionResultSessionCode);
		
		if (CurrentSessionResultSessionCode != SessionCodeToJoin)
		{
			continue;	
		}
		
		LOG_INFO(TEXT("Found session with code %s joining it"), *SessionCodeToJoin);
	
		ShowMessage(TEXT("Found the room"));
		
		if (GetSikSubsystem())
		{
			SikSubsystem->JoinSessions(CurrentSessionSearchResult);
		}
			
		return;
	}
	
	LOG_INFO(TEXT("Wrong Session Code Entered: %s"), *SessionCodeToJoin);
	
	ShowMessage(FString::Printf(TEXT("Wrong room Code Entered: %s"), *SessionCodeToJoin), true);
	
	bJoinSessionViaCode = false;
}

void USikHudWidget::UpdateSessionsList(const TArray<FOnlineSessionSearchResult>& Results)
{
	LOG_INFO(TEXT("Called"));

	TSet<FString> NewSessionKeys;
	bool bAnySessionExists = false;

	// Filter settings
	const auto [MapName, GameMode, 
		Players, RoomVisibility] = GetCurrentSessionsFilter();
	const bool bShowAllMap = MapName == "Any";
	const bool bShowAllGameMode = GameMode == "Any";
	const bool bShowAllPlayers = Players == "Any";

	// --- FIRST PASS: Add/update only filtered sessions ---
	for (const FOnlineSessionSearchResult& Result : Results)
	{
		if (Result.Session.NumOpenPublicConnections <= 0)
			continue;

		FSikCustomSessionSettings CurrentSessionSettings;
		Result.Session.SessionSettings.Get(SETTING_MAPNAME, CurrentSessionSettings.MapName);
		Result.Session.SessionSettings.Get(SETTING_GAMEMODE, CurrentSessionSettings.GameMode);
		Result.Session.SessionSettings.Get(SETTING_NUMPLAYERSREQUIRED, CurrentSessionSettings.Players);
		Result.Session.SessionSettings.Get(SETTING_SESSION_VISIBILITY, CurrentSessionSettings.Visibility);

		if (CurrentSessionSettings.Visibility == FString("Private"))
			continue;
		
		if (!bShowAllMap && CurrentSessionSettings.MapName != MapName)
			continue;
		
		if (!bShowAllGameMode && CurrentSessionSettings.GameMode != GameMode)
			continue;
		
		if (!bShowAllPlayers && CurrentSessionSettings.Players != Players)
			continue;
		
		// Session matches the filter → handle it
		const FString Key = Result.GetSessionIdStr();
		NewSessionKeys.Add(Key);

		// --- UPDATE EXISTING WIDGET ---
		if (USikSessionDataWidget** ExistingWidgetPtr = ActiveSessionWidgets.Find(Key))
		{
			(*ExistingWidgetPtr)->SetSessionInfo(Result, CurrentSessionSettings);
			bAnySessionExists = true;
			continue;
		}

		// --- ADD NEW WIDGET ---
		if (!SessionDataWidgetClass)
		{
			LOG_ERROR(TEXT("Please set the SessionDataWidgetClass in WBP_HudWidget_Sik!"));
			return;
		}

		USikSessionDataWidget* NewWidget = CreateWidget<USikSessionDataWidget>(GetWorld(), SessionDataWidgetClass);
		NewWidget->SetSessionInfo(Result, CurrentSessionSettings);
		NewWidget->SetSikHudWidget(this);

		AddSessionDataWidget(NewWidget);
		ActiveSessionWidgets.Add(Key, NewWidget);

		bAnySessionExists = true;
	}

	// --- SECOND PASS: REMOVE widgets NOT in filtered set ---
	for (const FString& CurrentKey : LastSessionKeys)
	{
		if (NewSessionKeys.Contains(CurrentKey))
			continue;

		if (USikSessionDataWidget** WidgetPtr = ActiveSessionWidgets.Find(CurrentKey))
		{
			if (USikSessionDataWidget* Widget = *WidgetPtr)
				Widget->RemoveFromParent();
		}

		ActiveSessionWidgets.Remove(CurrentKey);
	}

	LastSessionKeys = MoveTemp(NewSessionKeys);

	if (bAnySessionExists)
	{
		SetFindSessionsThrobberVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		SetFindSessionsThrobberVisibility(ESlateVisibility::Visible);
	}

	FindNewSessionsIfAllowed();
}

void USikHudWidget::FindNewSessionsIfAllowed()
{
	if (bCanFindNewSessions)
	{
		if (!IsValid(this) || !GetWorld() || GetWorld()->bIsTearingDown)
		{
			LOG_INFO(TEXT("UpdateSessionsList aborted – world is tearing down"));
			return;
		}

		if (GetSikSubsystem())
		{
			SikSubsystem->FindSessions();
		}
	}
}

void USikHudWidget::JoinTheGivenSession(FOnlineSessionSearchResult& InSessionToJoin)
{
	LOG_INFO(TEXT("Called"));
	
	if (!InSessionToJoin.IsValid())
	{
		LOG_ERROR(TEXT("InSessionToJoin is NULL!"));
		return;
	}
	
	ShowMessage(FString("Joining room"));
	
	bJoinSessionViaCode = false;
	bCanFindNewSessions = false;
	
	if (GetSikSubsystem())
	{
        SikSubsystem->CancelFindSessions();
		SikSubsystem->JoinSessions(InSessionToJoin);
	}
}

FText USikHudWidget::FilterEnteredSessionCode(const FText& InCode)
{
	const FString EnteredText = InCode.ToString();
    
	FString FilteredText;
	FilteredText.Reserve(SETTING_SESSION_CODELENGTH);

	for (const TCHAR Char : EnteredText)
	{
		if (FilteredText.Len() >= SETTING_SESSION_CODELENGTH)
		{
			break;
		}

		if (FChar::IsAlpha(Char))
		{
			FilteredText.AppendChar(FChar::ToUpper(Char));
		}
	}

	if (!FilteredText.Equals(EnteredText, ESearchCase::CaseSensitive))
	{
		return FText::FromString(FilteredText);
	}

	return InCode;
}

void USikHudWidget::StartFindingSessions()
{
	LOG_INFO(TEXT("Called"));
		
	ClearSessionsScrollBox();
	
	bCanFindNewSessions = true;
	
	ActiveSessionWidgets.Empty();
	
	LastSessionKeys.Empty();
	
	SetFindSessionsThrobberVisibility(ESlateVisibility::Visible);
	
	if (GetSikSubsystem())
	{
		SikSubsystem->FindSessions();
	}
}

void USikHudWidget::StopFindingSessions()
{	
	LOG_INFO(TEXT("Called"));
		
	ClearSessionsScrollBox();
		
	bCanFindNewSessions = false;
	
	ActiveSessionWidgets.Empty();
	
	LastSessionKeys.Empty();
	
	SetFindSessionsThrobberVisibility(ESlateVisibility::Visible);
}

TObjectPtr<USikSubsystem> USikHudWidget::GetSikSubsystem()
{
	if (IsValid(SikSubsystem))
	{
		return SikSubsystem;
	}
	
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		return SikSubsystem = GameInstance->GetSubsystem<USikSubsystem>();
	}

	LOG_ERROR(TEXT("Cannot validate SikSubsystem"));
	
	return nullptr;
}

#pragma endregion Defaults
	