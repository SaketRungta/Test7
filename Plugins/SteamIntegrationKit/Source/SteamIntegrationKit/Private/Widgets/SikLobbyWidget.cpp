// Copyright (c) 2025 The Unreal Guy. All rights reserved.

#include "Widgets/SikLobbyWidget.h"

#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameMode/SikLobbyGameMode.h"
#include "Online/OnlineSessionNames.h"
#include "System/SikLogger.h"

bool USikLobbyWidget::Initialize()
{
    LOG_INFO(TEXT("Called"));

    if (!Super::Initialize())
    {
        return false;
    }

    if (!IsDesignTime())
    {
        ASikLobbyGameMode::OnLobbyPlayersChangedGlobal.AddUObject(this, &ThisClass::OnLobbyPlayersChangedGlobal);

        if (GetSikSubsystem())
        {
            SikSubsystem->MultiplayerSessionsOnStartSessionComplete.AddDynamic(this, 
                &ThisClass::OnSessionStartedCallback);
        }

        if (const APlayerController* PC = GetOwningPlayer())
        {
            bIsHost = PC->HasAuthority();
        }
    }

    if (StartGameButton && PlayersCountDataHb)
    {
        StartGameButton->SetIsEnabled(false);

        if (bIsHost)
        {
            StartGameButton->SetVisibility(ESlateVisibility::Visible);
            PlayersCountDataHb->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            StartGameButton->SetVisibility(ESlateVisibility::Hidden);
            PlayersCountDataHb->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    if (!IsDesignTime())
    {
        OnLobbyPlayersChangedGlobal(1);
    }

    return true;
}

void USikLobbyWidget::NativeDestruct()
{
    LOG_INFO(TEXT("Called"));
    
    if (SikSubsystem)
    {
        SikSubsystem->MultiplayerSessionsOnStartSessionComplete.RemoveDynamic(this, 
            &ThisClass::OnSessionStartedCallback);
    }

    ASikLobbyGameMode::OnLobbyPlayersChangedGlobal.RemoveAll(this);
    
    Super::NativeDestruct();
}

void USikLobbyWidget::OnLobbyPlayersChangedGlobal(const uint32 CurrentPlayers)
{
    LOG_INFO(TEXT("Called"));
    
    if (!GetSikSubsystem())
    {
        LOG_ERROR(TEXT("SikSubsystem is NULL"));
        return;
    }
    
    if (int32 MaxPlayers = 0; SikSubsystem->GetMaxPlayers(MaxPlayers))
    {
        if (PlayerCountText)
        {
            const FString CountString = FString::Printf(TEXT("%d / %d"), CurrentPlayers, MaxPlayers);
            PlayerCountText->SetText(FText::FromString(CountString));
        }

        if (bIsHost && StartGameButton)
        {
            const bool bLobbyFull = (MaxPlayers > 0) && (CurrentPlayers >= static_cast<uint32>(MaxPlayers));
            StartGameButton->SetIsEnabled(bLobbyFull);
        }
    }
    else
    {
        if (PlayerCountText)
        {
            PlayerCountText->SetText(FText::FromString(TEXT("-- / --")));
        }

        if (bIsHost && StartGameButton)
        {
            StartGameButton->SetIsEnabled(false);
        }
    }
    
    if (bIsOtherSessionSettingsSet)
    {
        return;
    }
    
    if (SessionCodeText)
    {
        FString Setting;
        SikSubsystem->GetSessionSetting(SETTING_SESSIONKEY, Setting)
        ? SessionCodeText->SetText(FText::FromString(Setting))
        : SessionCodeText->SetText(FText::FromString(TEXT("------")));
    }
    
    if (SessionVisibilityText)
    {
        FString Setting;
        SikSubsystem->GetSessionSetting(SETTING_SESSION_VISIBILITY, Setting)
        ? SessionVisibilityText->SetText(FText::FromString(Setting))
        : SessionVisibilityText->SetText(FText::FromString(TEXT("------")));
    }
    
    if (GameModeText)
    {
        FString Setting;
        SikSubsystem->GetSessionSetting(SETTING_GAMEMODE, Setting)
        ? GameModeText->SetText(FText::FromString(Setting))
        : GameModeText->SetText(FText::FromString(TEXT("------")));
    }
    
    if (MapNameText)
    {
        FString Setting;
        SikSubsystem->GetSessionSetting(SETTING_MAPNAME, Setting)
        ? MapNameText->SetText(FText::FromString(Setting))
        : MapNameText->SetText(FText::FromString(TEXT("------")));
    }
    
    bIsOtherSessionSettingsSet = true;
}

void USikLobbyWidget::OnStartGameClicked()
{
    LOG_INFO(TEXT("Called")); 

    if (!bIsHost)
    {
        LOG_WARNING(TEXT("Non-host tried to start game"));
        return;
    }

    if (GetSikSubsystem())
    {
        SikSubsystem->StartSession();
    }
}

void USikLobbyWidget::OnSessionStartedCallback(bool bWasSuccessful)
{
    LOG_INFO(TEXT("Start sessions completed : %s"),
        bWasSuccessful ? TEXT("Success") : TEXT("Failed"));

    if (!bWasSuccessful)
    {
        LOG_ERROR(TEXT("Failed to start session"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        LOG_ERROR(TEXT("World is NULL"));
        return;
    }

    if (MapPaths.IsEmpty())
    {
        LOG_ERROR(TEXT("MapPaths is EMPTY"));
        return;
    }

    if (MapPaths.Contains(MapNameText->GetText().ToString()))
    {
        LOG_INFO(TEXT("ServerTravel to: %s"), *MapPaths[MapNameText->GetText().ToString()]);

        World->ServerTravel(MapPaths[MapNameText->GetText().ToString()]);
    }
    else
    {
        LOG_ERROR(TEXT("MapPaths does not contain the map for the currently selected map"));
    }
}

TObjectPtr<USikSubsystem> USikLobbyWidget::GetSikSubsystem()
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
